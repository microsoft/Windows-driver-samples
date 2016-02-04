#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)
#if (P2PSVC_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
u4Byte
p2psvc_EvalSearchResultObjListLen(
	IN  POCTET_STRING						posP2PAttrs,
	IN  OCTET_STRING						osAdvSvcInfo,
	IN  u4Byte								svcInfoLen,
	OUT pu1Byte								pnAdvSvc
	)
{
	u4Byte									reqBufSize = 0;

	OCTET_STRING 							osDevName = {NULL, 0};

	u4Byte									totalSvcNameLen_1 = 0;
	u4Byte									totalSvcNameLen_2 = 0;
	u1Byte									nAdvSvc = 0;

	do
	{
		u4Byte 								idxAdvSvcInfoBuf = 0;

		if(RT_STATUS_SUCCESS != P2PSvc_GetDevNameFromDevInfoAttr(posP2PAttrs, &osDevName))
		{
			break;
		}

		//RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "osAdvSvcInfo:\n", osAdvSvcInfo.Octet, osAdvSvcInfo.Length);

		while(idxAdvSvcInfoBuf < osAdvSvcInfo.Length)
		{
			u1Byte							curSvcNameLen = 0;

			// adv-id
			idxAdvSvcInfoBuf += 4; 

			// config-method (addendm v0.43)
			idxAdvSvcInfoBuf += 2;

			// svc-name-len
			curSvcNameLen = (*(pu1Byte)(osAdvSvcInfo.Octet + idxAdvSvcInfoBuf));
			totalSvcNameLen_2 += curSvcNameLen;
			idxAdvSvcInfoBuf += 1; 																				

			// svc-name
			//RT_PRINT_STR(COMP_P2P, DBG_LOUD, "svc-name:\n", osAdvSvcInfo.Octet + i, curSvcNameLen);
			idxAdvSvcInfoBuf += curSvcNameLen;

			// TODO: shall check if the svc matches our req because hashes of other seek req may be carried in our ProbeReq so we may get more svc names then we actually want
			
			nAdvSvc++;
		}

		totalSvcNameLen_1 = osAdvSvcInfo.Length - (nAdvSvc * (4 + 2 + 1)); // minus adv-id(4), config-method(2) and svc-name-length(1)

		if(totalSvcNameLen_1 != totalSvcNameLen_2) 
		{
			//RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] totalSvcNameLen_1(%u) != totalSvcNameLen_2(%u)\n", totalSvcNameLen_1, totalSvcNameLen_2));
			//RT_PRINT_DATA(COMP_P2P, DBG_WARNING, "osAdvSvcInfo:\n", osAdvSvcInfo.Octet, osAdvSvcInfo.Length);
			break;
		}
		
		reqBufSize = 0;
		reqBufSize += FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart);
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 1); 											// search-id
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);											// dev-addr
		reqBufSize += (RT_OBJECT_HEADER_SIZE + osDevName.Length);							// dev-name, no more than 32
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u1Byte));								// svc count
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));							// is SD done
		reqBufSize += nAdvSvc * (FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart));
		reqBufSize += nAdvSvc * (RT_OBJECT_HEADER_SIZE + 4);								// adv-id
		reqBufSize += nAdvSvc * (RT_OBJECT_HEADER_SIZE + 2);								// config-method (addendum v0.43)
		reqBufSize += (nAdvSvc * (RT_OBJECT_HEADER_SIZE)) + totalSvcNameLen_1; 				// svc-name

		if(pnAdvSvc) *pnAdvSvc = nAdvSvc;
	}while(FALSE);	
	
	return reqBufSize;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------


VOID
P2PSvc_Free_SearchResultObjList(
	IN  PRT_LIST_ENTRY						pListHead,
	IN  pu4Byte								pListCnt
	)
{
	while(RTIsListNotEmpty(pListHead))
	{
		PVOID 								pEntry = NULL;
		PP2PSVC_SR_LIST_ENTRY 				pSREntry = NULL;
	
		pEntry = RTRemoveHeadListWithCnt(pListHead, pListCnt);
		pSREntry = (PP2PSVC_SR_LIST_ENTRY)pEntry;
		P2PSvc_FreeMem(pEntry, P2PSVC_SR_LIST_LEN(pSREntry));
	}
}

BOOLEAN
P2PSvc_ToProcessProbeRsp(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN	 u1Byte								searchId,
	IN  pu1Byte								devAddr
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_SR_LIST_ENTRY 					pSREntryInList = NULL;
	BOOLEAN									bAccept = FALSE;

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_GetSearchResult(pP2PSvcInfo, searchId, devAddr, &pSREntryInList)))
		{
			break;
		}

		if(NULL == pSREntryInList)
		{
			bAccept = TRUE;
			break;
		}
		else
		{// already have an entry in search result list
			u8Byte							curTime = PlatformGetCurrentTime();

			RT_TRACE_F(COMP_P2P, DBG_TRACE, ("curTime: %I64u, lastProbeRspTime: %I64u\n", curTime, pSREntryInList->probeRspRxTime));

			// Check if we receive a bunch of probe rsp in a very short time
			if(curTime - pSREntryInList->probeRspRxTime < P2PSVC_LEAST_PROBE_RSP_TIME_uS)
			{
				break;
			}

			// Check if we have a outstanding SD req, if we process the probe rsp in this case, we'll send another SD req and may flood the channel
			if(0 != pSREntryInList->sdReqTxTime && 0 == pSREntryInList->sdRspRxTime)
			{// we've sent SD req but SD rsp is not yet recvd
				// Check if we've recv SD rsp within timeout
				if(P2PSVC_TIMEOUT_WAIT_SD_RSP_uS < curTime - pSREntryInList->sdReqTxTime)
				{
					// Prefast warning
#pragma warning( disable:6328 )
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Timeout waiting for SD rsp, try again by accept probe rsp, sdReqTxTime: %u\n", pSREntryInList->sdReqTxTime));
					RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Target dev addr:\n", devAddr);
					
					pSREntryInList->sdReqTxTime = 0;
					bAccept = TRUE;
					break;
				}
				else
				{// still waiting for SD rsp
					break;
				}
			}

			// Check if we've received a SD rsp from peer and we don't want the content of this probe rsp to cover the content of SD rsp
			if(0 != pSREntryInList->sdRspRxTime)
			{// we've received a SD rsp
				RT_TRACE_F(COMP_P2P, DBG_TRACE, ("Drop for we got SD rsp already\n"));
				break;
			}
		}

	}while(FALSE);

	if(bAccept)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Accept probe rsp\n"));
	}

	return bAccept;
}



//
// Description:
// 	Search the search result list with search id and devAddr as the key.
// 	If entry found, the pointer to the entry is returned.
// 	If not found, the pointer is set to NULL.
//
RT_STATUS
P2PSvc_GetSearchResult(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN  u1Byte								searchId,
	IN  pu1Byte								devAddr,
	OUT PP2PSVC_SR_LIST_ENTRY				*ppSREntry
	)
{
		RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
		PRT_LIST_ENTRY 						pEntry = NULL;
	
		if(ppSREntry) *ppSREntry = NULL;
	
		do
		{
			for(pEntry = RTGetHeadList(&pP2PSvcInfo->searchResultList); pEntry != &pP2PSvcInfo->searchResultList; pEntry = RTNextEntryList(pEntry))
			{
				PP2PSVC_SR_LIST_ENTRY 	pSREntry = (PP2PSVC_SR_LIST_ENTRY)pEntry;
				PRT_OBJECT_HEADER 		pSearchIdObj = NULL;
				PRT_OBJECT_HEADER 		pDevAddrObj = NULL;
	
				// Match search id
				if(NULL == (pSearchIdObj = P2PSvc_GetParam(&pSREntry->srObjList, P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 0)))
				{
					rtStatus = RT_STATUS_INVALID_DATA;
					break;
				}
	
				if(searchId != *(pu1Byte)pSearchIdObj->Value)
				{
					continue;;
				}
	
				// Match dev addr
				if(NULL == (pDevAddrObj = P2PSvc_GetParam(&pSREntry->srObjList, P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, 0)))
				{
					rtStatus = RT_STATUS_INVALID_DATA;
					break;
				}
				if(!eqMacAddr(devAddr, pDevAddrObj->Value))
				{
					continue;
				}
	
				// Target found
				if(ppSREntry) *ppSREntry = pSREntry;
				break;
			}
	
			if(RT_STATUS_SUCCESS != rtStatus) break;
		}while(FALSE);
	
		return rtStatus;
	}


BOOLEAN
P2PSvc_SeekReqNeedSD(
	IN  PP2PSVC_OBJ_LIST					pSeekObjList,
	IN  PP2PSVC_SR_LIST_ENTRY				pSREntryInList,
	OUT PRT_OBJECT_HEADER					*ppSvcInfoReqObj
	)
{
	PRT_OBJECT_HEADER						pSeekSvcInfoReqObj = NULL;
	PRT_OBJECT_HEADER						pSvcDescObjList = NULL;
	PRT_OBJECT_HEADER						pSvcInfoObj = NULL;
	BOOLEAN									bNeedSD = FALSE;

	do
	{
		//4 Check if seek req contains svc-info-req
		if(NULL == (pSeekSvcInfoReqObj = P2PSvc_GetParam(pSeekObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO_REQ, 0)))
		{// shall not happen
			break;
		}
		
		if(0 == pSeekSvcInfoReqObj->Length)
		{
			PRT_OBJECT_HEADER 				pSeekSvcNameObj = NULL;
			if(NULL == (pSeekSvcNameObj = P2PSvc_GetParam(pSeekObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 0)))
			{// shall not happen
				break;
			}
		
			if('*' == *((pu1Byte)pSeekSvcNameObj->Value + pSeekSvcNameObj->Length - 1))
			{// ended with *, assume UTF-8 encode of * uses only a single byte
				bNeedSD = TRUE;
				break;
			}

			break;
		}
		
		//4 Check if we've already done SD
		// Get a svc desc
		if(NULL == (pSvcDescObjList = P2PSvc_GetParam(&pSREntryInList->srObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_DESC_LIST, 0)))
		{
			break;
		}
		
		// Check if the svc desc has svc-info
		if(NULL == (pSvcInfoObj = P2PSvc_GetParam((PP2PSVC_OBJ_LIST)pSvcDescObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO, 0)))
		{
			bNeedSD = TRUE;
			break;
		}
		else
		{// already got svc-info from peer by SD
			break;
		}

	}while(FALSE);

	if(bNeedSD)
	{
		if(ppSvcInfoReqObj) *ppSvcInfoReqObj = pSeekSvcInfoReqObj;
	}

	return bNeedSD;
}

//
// Description:
//		Update search result list if 
//		1. the input entry can be found in the list and the content is different
//		2. the input entry can't be found in the list
//
// 	Return the entry in the list if no error occurs.
//
RT_STATUS
P2PSvc_UpdateSearchResult(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN	 u1Byte								searchId, 
	IN  pu1Byte								devAddr,
	IN  PP2PSVC_SR_LIST_ENTRY				pSREntryToUpdate,
	OUT PBOOLEAN							pbDuplicatedEntry,
	OUT PP2PSVC_SR_LIST_ENTRY				*ppSREntryOut
	)
{
	RT_STATUS 								rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_SR_LIST_ENTRY					pSREntryInList = NULL;

	do
	{
		if(ppSREntryOut) *ppSREntryOut = NULL;

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_GetSearchResult(pP2PSvcInfo, searchId, devAddr, &pSREntryInList)))
		{
			break;
		}

		if(pSREntryInList)
		{// remove the old one and insert the new to the head	

			if(P2PSVC_OBJ_LIST_LEN(&pSREntryToUpdate->srObjList) == P2PSVC_OBJ_LIST_LEN(&pSREntryInList->srObjList) &&
				0 == PlatformCompareMemory(&pSREntryToUpdate->srObjList, &pSREntryInList->srObjList, P2PSVC_OBJ_LIST_LEN(&pSREntryToUpdate->srObjList)))

			{// has same entry in the list
				if(ppSREntryOut) *ppSREntryOut = pSREntryInList;
				if(pbDuplicatedEntry) *pbDuplicatedEntry = TRUE;
			}
			else
			{// different => replace the entry
				PRT_LIST_ENTRY		pEntry = (PRT_LIST_ENTRY)pSREntryInList;
				PRT_LIST_ENTRY 		pForeEntry = RTForeEntryList(pEntry);

				RTRemoveEntryListWithCnt(pEntry, &pP2PSvcInfo->searchResultListCnt);
				P2PSvc_FreeMem(pSREntryInList, P2PSVC_SR_LIST_LEN(pSREntryInList));
				pEntry = pForeEntry;
				RTInsertHeadListWithCnt(&pP2PSvcInfo->searchResultList, &pSREntryToUpdate->List, &pP2PSvcInfo->searchResultListCnt);

				RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Update SR entry:\n", pSREntryToUpdate, P2PSVC_SR_LIST_LEN(pSREntryToUpdate));

				if(ppSREntryOut) *ppSREntryOut = pSREntryToUpdate;
			}
		}
		else
		{// entry not found in the search result list
			RTInsertHeadListWithCnt(&pP2PSvcInfo->searchResultList, &pSREntryToUpdate->List, &pP2PSvcInfo->searchResultListCnt);
			if(ppSREntryOut) *ppSREntryOut = pSREntryToUpdate;
		}
	}while(FALSE);

	return rtStatus;

}

//
// Description:
//		Make search result data from ProbeRsp, memory is allocated,
//		svc-info is empty.
//
//		Fixed fields including:
//			search-id, dev-addr, dev-name, 
//
//		Var fields describing each services advertised by the dev are:
//			adv-id, svc-name, (no svc-info), svc-status
//
// 	The advertised svc desc carried in advertised svc info attr contains:
//			adv-id, svc-name-len, svc-name
//
// Indicate seek result event including the following info:
// 1. search-id (4)
// 2. dev-addr (6)
// 3. dev-name (n, n <= 32)
//----------------------------
// 4. adv-id (4)
// 5. config-method (2) (addendum v0.43)
// 6. svc-name (n, n <= 255)
// 7. svc-info (n, n <= 65535)
// 8. svc-status (1) 
//
RT_STATUS
P2PSvc_MakeSearchResultDataFromProbeRsp(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN  PP2PSVC_REQ_INFO_ENTRY 				pSeekInfoEntry, 
	IN  pu1Byte								devAddr, 
	IN  POCTET_STRING						posP2PAttrs,
	IN  OCTET_STRING						osAdvSvcInfo,
	OUT PP2PSVC_SR_LIST_ENTRY				*ppSREntryOut
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_OBJ_LIST						pObjList = NULL;
	PP2PSVC_SR_LIST_ENTRY					pSREntry = NULL;
	u4Byte									reqBufSize = 0;

	P2PSVC_CHECK_NULL(ppSREntryOut);

	P2PSVC_FUNC_IN(DBG_TRACE);

	do
	{	
		u4Byte 								idxAdvSvc = 0, idxAdvSvcInfoBuf = 0; 
		u1Byte								nAdvSvc = 0; 

		*ppSREntryOut = NULL;
		
		if(0 == (reqBufSize = p2psvc_EvalSearchResultObjListLen(posP2PAttrs, osAdvSvcInfo, 0, &nAdvSvc)))
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		reqBufSize += FIELD_OFFSET(P2PSVC_SR_LIST_ENTRY, srObjList);

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pSREntry, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pSREntry\n"));
			break;
		}
		
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("alloc mem for size: %u\n", reqBufSize));

		PlatformFillMemory(pSREntry, reqBufSize, 0xFE);
		pSREntry->probeRspRxTime = 0;
		pSREntry->sdReqTxTime = 0;
		pSREntry->sdRspRxTime = 0;
		pSREntry->bDirty = FALSE;

		pObjList = &pSREntry->srObjList;

		P2PSVC_OBJ_LIST_INIT(pObjList, 
					RT_OB_HDR_TYPE_DATA, 
					P2PSVC_OBJ_HDR_ID_INDIC_SEARCH_RESULT, 
					P2PSVC_MIN_SUPPORT_VER);

		//1 Note that because we have search id in the obj, the search result would belong to a specific seek req

		// Make required objs, nObjs is updated accordingly
		if(0 == P2PSvc_MakeSearchIdObj(pObjList, pSeekInfoEntry, NULL)) 		{rtStatus = RT_STATUS_INVALID_DATA ; break;}
		if(0 == P2PSvc_MakeDevAddrObj(pObjList, devAddr))						{rtStatus = RT_STATUS_INVALID_DATA ; break;}
		if(0 == P2PSvc_MakeDevNameObj_FromP2PAttrs(pObjList, posP2PAttrs))		{rtStatus = RT_STATUS_INVALID_DATA ; break;}
		if(0 == P2PSvc_MakeSvcCountObj(pObjList, nAdvSvc))						{rtStatus = RT_STATUS_INVALID_DATA ; break;}
		if(0 == P2PSvc_MakeIsSDDoneObj(pObjList, FALSE))						{rtStatus = RT_STATUS_INVALID_DATA ; break;}

		// Make svc desc obj list for each svc found
		for(idxAdvSvc = 0, idxAdvSvcInfoBuf = 0; idxAdvSvc < nAdvSvc; idxAdvSvc++)
		{
			PP2PSVC_OBJ_LIST 				pSvcDescObjList = NULL;
			u1Byte							svcNameLen = 0;

			if(0 == P2PSvc_MakeSvcDescObjList(pObjList, &pSvcDescObjList)) 		{rtStatus = RT_STATUS_INVALID_DATA ; break;}

			// adv-id
			P2PSvc_MakeAdvIdObj(pSvcDescObjList, *((pu4Byte)(osAdvSvcInfo.Octet + idxAdvSvcInfoBuf)));
			idxAdvSvcInfoBuf += 4;

			// config-method (addendum v0.43)
			P2PSvc_MakeConfigMethodObj(pSvcDescObjList, *((pu2Byte)(osAdvSvcInfo.Octet + idxAdvSvcInfoBuf)));
			idxAdvSvcInfoBuf += 2;

			svcNameLen = *((pu1Byte)(osAdvSvcInfo.Octet + idxAdvSvcInfoBuf));
			idxAdvSvcInfoBuf += 1;

			// svc-name
			P2PSvc_MakeSvcNameObj(pSvcDescObjList, svcNameLen, osAdvSvcInfo.Octet + idxAdvSvcInfoBuf);
			idxAdvSvcInfoBuf += svcNameLen;

			pObjList->hdr.Length += P2PSVC_OBJ_LIST_VAR_LEN(pSvcDescObjList);
		}

		if(RT_STATUS_SUCCESS != rtStatus)
		{
			break;
		}

		*ppSREntryOut = pSREntry;

		RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2PSvc_MakeSearchResultDataFromProbeRsp():\n", pObjList, P2PSVC_OBJ_LIST_LEN(pObjList));
		
	}while(FALSE);

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		if(pSREntry) P2PSvc_FreeMem(pSREntry, reqBufSize);
		*ppSREntryOut = NULL;
	}

	P2PSVC_FUNC_OUT(DBG_TRACE, rtStatus);

	return rtStatus;
}

RT_STATUS
P2PSvc_MakeSearchResultDataFromSDRsp(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN  u1Byte								searchId,
	IN  pu1Byte								devAddr,
	IN  PP2P_SERVICE_RSP_TLV				pSvcRspTlv,
	OUT PP2PSVC_SR_LIST_ENTRY				*ppSREntryOut
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_SR_LIST_ENTRY					pOldSREntry = NULL;
	PP2PSVC_SR_LIST_ENTRY					pNewSREntry = NULL;
	PP2PSVC_OBJ_LIST						pOldObjList = NULL;
	PP2PSVC_OBJ_LIST						pNewObjList = NULL;
	u4Byte									reqBufSize = 0;

	P2PSVC_CHECK_NULL(ppSREntryOut);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		u1Byte								svcNameLen = 0;
		pu1Byte								pSvcNameBuf = NULL;
		u4Byte								advId = 0;
		u1Byte								svcStatus = 0;
		u2Byte								svcInfoLen = 0;
		pu1Byte								pSvcInfoBuf = NULL;

		u1Byte								nSvcInfoDesc = 0;
		u4Byte								totalSvcNameLen = 0;
		u4Byte								totalSvcInfoLen = 0;

		PRT_OBJECT_HEADER					pSearchIdObj = NULL;
		PRT_OBJECT_HEADER					pDevAddrObj = NULL;
		PRT_OBJECT_HEADER					pDevNameObj = NULL;
		PRT_OBJECT_HEADER					pIsSDDoneObj = NULL;

		u1Byte								idxSvcInfoDesc = 0;

		while(RT_STATUS_SUCCESS == P2PSvc_ParseAnqpQueryRsp(pSvcRspTlv, nSvcInfoDesc, &svcNameLen, &pSvcNameBuf, &advId, &svcStatus, &svcInfoLen, &pSvcInfoBuf))
		{
			totalSvcNameLen += svcNameLen;
			totalSvcInfoLen += svcInfoLen;
			nSvcInfoDesc++;
		}

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_GetSearchResult(pP2PSvcInfo, searchId, devAddr, &pOldSREntry)))
		{
			break;
		}

		pOldObjList = &pOldSREntry->srObjList;

		pSearchIdObj = P2PSvc_GetParam(pOldObjList, P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 0);
		pDevAddrObj = P2PSvc_GetParam(pOldObjList, P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, 0);
		pDevNameObj = P2PSvc_GetParam(pOldObjList, P2PSVC_OBJ_HDR_ID_DATA_DEV_NAME, 0);
		pIsSDDoneObj = P2PSvc_GetParam(pOldObjList, P2PSVC_OBJ_HDR_ID_DATA_IS_SD_DONE, 0);

		if(pIsSDDoneObj && FALSE == *(PBOOLEAN)pIsSDDoneObj->Value)
		{
			*(PBOOLEAN)pIsSDDoneObj->Value = TRUE;
		}

		reqBufSize = 0;
		reqBufSize += FIELD_OFFSET(P2PSVC_SR_LIST_ENTRY, srObjList);
		reqBufSize += FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart);
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 1);													// search-id
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);													// dev-addr
		reqBufSize += (RT_OBJECT_HEADER_SIZE + pDevNameObj->Length); 							// dev-name, no more than 32
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 1);													// svc count
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));									// is SD done
		reqBufSize += nSvcInfoDesc * (FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart));
		reqBufSize += nSvcInfoDesc * (RT_OBJECT_HEADER_SIZE + 4);								// adv-id
		reqBufSize += (nSvcInfoDesc * (RT_OBJECT_HEADER_SIZE)) + totalSvcNameLen;			// svc-name
		reqBufSize += (nSvcInfoDesc * (RT_OBJECT_HEADER_SIZE)) + totalSvcInfoLen; 			// svc-info
		reqBufSize += nSvcInfoDesc * (RT_OBJECT_HEADER_SIZE + 1);								// svc-status	

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pNewSREntry, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pSREntry\n"));
			break;
		}

		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("alloc mem for size: %u\n", reqBufSize));

		PlatformFillMemory(pNewSREntry, reqBufSize, 0xFE);
		pNewSREntry->probeRspRxTime = pOldSREntry->probeRspRxTime;
		pNewSREntry->sdReqTxTime = 0;
		pNewSREntry->sdRspRxTime = 0;
		pNewSREntry->bDirty = FALSE;

		pNewObjList = &pNewSREntry->srObjList;
		
		P2PSVC_OBJ_LIST_INIT(pNewObjList, 
					RT_OB_HDR_TYPE_DATA, 
					P2PSVC_OBJ_HDR_ID_INDIC_SEARCH_RESULT, 
					P2PSVC_MIN_SUPPORT_VER);

		P2PSvc_MakeObj(pNewObjList, P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, pSearchIdObj->Length, pSearchIdObj->Value);
		P2PSvc_MakeObj(pNewObjList, P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, pDevAddrObj->Length, pDevAddrObj->Value);
		P2PSvc_MakeObj(pNewObjList, P2PSVC_OBJ_HDR_ID_DATA_DEV_NAME, pDevNameObj->Length, pDevNameObj->Value);
		P2PSvc_MakeObj(pNewObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_COUNT, sizeof(nSvcInfoDesc), &nSvcInfoDesc);

		if(pIsSDDoneObj != NULL)
			P2PSvc_MakeObj(pNewObjList, P2PSVC_OBJ_HDR_ID_DATA_IS_SD_DONE, sizeof(BOOLEAN), pIsSDDoneObj->Value);
					
		for(idxSvcInfoDesc = 0; idxSvcInfoDesc < nSvcInfoDesc; idxSvcInfoDesc++)
		{
			PP2PSVC_OBJ_LIST				pSvcDescObjList = NULL;
			u1Byte							svcNameLen = 0;
		
			if(0 == P2PSvc_MakeSvcDescObjList(pNewObjList, &pSvcDescObjList)) {rtStatus = RT_STATUS_INVALID_DATA ; break;}

			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_ParseAnqpQueryRsp(pSvcRspTlv, idxSvcInfoDesc, &svcNameLen, &pSvcNameBuf, &advId, &svcStatus, &svcInfoLen, &pSvcInfoBuf)))
			{
				break;
			}

			P2PSvc_MakeObj(pSvcDescObjList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, sizeof(advId), &advId);
			P2PSvc_MakeObj(pSvcDescObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, svcNameLen, pSvcNameBuf);
			P2PSvc_MakeObj(pSvcDescObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO, svcInfoLen, pSvcInfoBuf);
			P2PSvc_MakeObj(pSvcDescObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS, sizeof(svcStatus), &svcStatus);

			pNewObjList->hdr.Length += P2PSVC_OBJ_LIST_VAR_LEN(pSvcDescObjList);
		}

		if(RT_STATUS_SUCCESS != rtStatus)
		{
			break;
		}

		*ppSREntryOut = pNewSREntry;

		// Free the old one
		RTRemoveEntryListWithCnt((PRT_LIST_ENTRY)pOldSREntry, &pP2PSvcInfo->searchResultListCnt);
		P2PSvc_FreeMem(pOldSREntry, P2PSVC_SR_LIST_LEN(pOldSREntry));
		
	}while(FALSE);

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		if(pNewSREntry) P2PSvc_FreeMem(pNewSREntry, reqBufSize);
		*ppSREntryOut = NULL;
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)
