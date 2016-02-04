#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)
#if (P2PSVC_SUPPORT == 1)

u4Byte gP2PSvcMemAllocCount = 0;
u4Byte gP2PSvcMemFreeCount = 0;


RT_STATUS
P2PSvc_AllocMem(
	IN	 PVOID								Adapter,
	OUT PVOID								*ppPtr,
	IN	 u4Byte								len
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	
	rtStatus = PlatformAllocateMemory(Adapter, ppPtr, len);
	if(RT_STATUS_SUCCESS == rtStatus) gP2PSvcMemAllocCount++;
	else rtStatus = RT_STATUS_RESOURCE;

	return rtStatus;
}

VOID
P2PSvc_FreeMem(
	IN  PVOID								pPtr,
	IN	 u4Byte								len
	)
{
	PlatformFreeMemory(pPtr, len);
	gP2PSvcMemFreeCount++;
}

RT_STATUS
P2PSvc_GetAdvSvcByAdvId(
	IN  PP2PSVC_INFO					pP2PSvcInfo, 
	IN  u4Byte							advId,
	OUT PP2PSVC_REQ_INFO_ENTRY 			*ppAdvSvcEntry
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	
	do
	{
		PRT_LIST_ENTRY 					pEntry = NULL;

		for(pEntry = RTGetHeadList(&pP2PSvcInfo->advSvcList); pEntry != &pP2PSvcInfo->advSvcList; pEntry = RTNextEntryList(pEntry))
		{
			PP2PSVC_REQ_INFO_ENTRY		pInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
			PRT_OBJECT_HEADER			pObj = NULL;
				
			pObj = P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
			if(sizeof(u4Byte) != pObj->Length) {rtStatus = RT_STATUS_INVALID_LENGTH; break;}
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("compare peer adv-id(%u) with self (%u)\n", advId, *(pu4Byte)pObj->Value));
			if(advId != *(pu4Byte)pObj->Value) continue;

			if(ppAdvSvcEntry) *ppAdvSvcEntry = pInfoEntry;
			break;
		}

		if(RT_STATUS_SUCCESS != rtStatus)
		{
			break;
		}

	}while(FALSE);

	return rtStatus;

}

BOOLEAN
P2PSvc_GetP2PAttr(
	IN  POCTET_STRING						posP2PAttrs,
	IN  u1Byte								attrId,
	IN  u4Byte								minLen,
	OUT POCTET_STRING						posAttr
	)
{
	RT_STATUS								status = RT_STATUS_SUCCESS;
	u1Byte									*pos = NULL;
	u1Byte									*end = NULL;
	
	posAttr->Length = 0;
	posAttr->Octet = NULL;

	pos = posP2PAttrs->Octet;
	end = pos + posP2PAttrs->Length;

	while(pos < end)
	{
		u2Byte					attrLen = 0;

		if(RT_STATUS_SUCCESS != (status = p2p_parse_ValidateAttribute(pos, end, &attrLen)))
		{
			RT_PRINT_DATA(COMP_P2P, DBG_WARNING, "rx invalid attribute\n", posP2PAttrs->Octet, posP2PAttrs->Length);
			break;
		}
		
		// Parse the attribute
		if(attrId == pos[0])
		{
			posAttr->Length = ReadEF2Byte(pos + 1);
			posAttr->Octet = pos + 3;
			break;
		}

		// Parse next attribute
		pos += 3 + attrLen;
	}

	return (posAttr->Octet) ? (TRUE) : (FALSE);
}

//
// Description:
// 	To indicate event with associated data (represented in object list) to upper layer.
//
RT_STATUS
P2PSvc_Indicate(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN  u4Byte								bufLen,
	IN  PVOID								pvBuf
	)
{
	RT_STATUS 								rtStatus = RT_STATUS_SUCCESS;

	do
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("indicate a buf with length: %u\n", bufLen));
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "indicated data:\n", pvBuf, bufLen);
		
		if(RT_STATUS_SUCCESS != (rtStatus = PlatformIndicateCustomStatus(pP2PSvcInfo->pAdapter, 
												RT_CUSTOM_EVENT_P2P_SVC, 
												RT_CUSTOM_INDI_TARGET_IHV, 
												pvBuf, bufLen)
										)
			)
		{
			break;
		}

	}while(FALSE);

	return rtStatus;

}

RT_STATUS
P2PSvc_IndicateFrameSent(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN  u4Byte								id,
	IN  pu1Byte								peerMac,
	IN  u2Byte								attrLen,
	IN  pu1Byte								pAttrs
	)
{
	RT_STATUS 								rtStatus = RT_STATUS_SUCCESS;
	PVOID										pBuf = NULL;
	u4Byte									reqBufSize = 0;

	do
	{
		PP2PSVC_OBJ_LIST					pObjList = NULL;

		reqBufSize = 0
				+ FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart)
				+ (RT_OBJECT_HEADER_SIZE + 6)
				+ (RT_OBJECT_HEADER_SIZE + attrLen)
				;
		

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuff\n"));
			break;
		}
		
		pObjList = (PP2PSVC_OBJ_LIST)pBuf;
		P2PSVC_OBJ_LIST_INIT(pObjList, RT_OB_HDR_TYPE_DATA, id, P2PSVC_MIN_SUPPORT_VER);

		P2PSvc_MakeDevAddrObj(pObjList, peerMac);
		P2PSvc_MakeP2PAttrsObj(pObjList, attrLen, pAttrs);
		
		if(RT_STATUS_SUCCESS != (rtStatus = PlatformIndicateCustomStatus(pP2PSvcInfo->pAdapter, 
												RT_CUSTOM_EVENT_P2P_SVC, 
												RT_CUSTOM_INDI_TARGET_IHV, 
												pObjList, reqBufSize)
										)
			)
		{
			break;
		}

	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	return rtStatus;

}


//
// Description:
// 	To indicate event with no data to upper layer.
//
RT_STATUS
P2PSvc_Notify(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN  u4Byte								id
	)
{
	RT_STATUS 								rtStatus = RT_STATUS_SUCCESS;
	PVOID										pBuf = NULL;
	u4Byte									reqBufSize = 0;

	do
	{	
		PP2PSVC_OBJ_LIST					pObjList = NULL;

		reqBufSize = 0;
		reqBufSize += FIELD_OFFSET(P2PSVC_OBJ_LIST, ObjTab);

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuff\n"));
			break;
		}
		
		pObjList = (PP2PSVC_OBJ_LIST)pBuf;
		P2PSVC_OBJ_LIST_INIT(pObjList, RT_OB_HDR_TYPE_DATA, id, P2PSVC_MIN_SUPPORT_VER);
		
		if(RT_STATUS_SUCCESS != (rtStatus = PlatformIndicateCustomStatus(pP2PSvcInfo->pAdapter, 
												RT_CUSTOM_EVENT_P2P_SVC, 
												RT_CUSTOM_INDI_TARGET_IHV, 
												pObjList, reqBufSize)
										)
			)
		{
			break;
		}

	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	return rtStatus;

}

BOOLEAN
P2PSvc_SeekReqIsBcst(
	IN  PP2PSVC_OBJ_LIST					pObjList
	)
{
	PRT_OBJECT_HEADER						pSeekDevAddrObj = NULL;
	BOOLEAN 									bIsBcst = FALSE;

	do
	{
		pSeekDevAddrObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, 0);
		if(MacAddr_isBcst(pSeekDevAddrObj->Value))
		{
			bIsBcst = TRUE;
		}
	}while(FALSE);

	return bIsBcst;
}

BOOLEAN
P2PSvc_MatchSubstring(
	IN  pu1Byte								targetBuf,
	IN  u4Byte								targetBufLen,
	IN  pu1Byte								patternBuf,
	IN  u4Byte								patternBufLen
	)
{
	u4Byte 									targetIdx = 0;
	u4Byte 									patternIdx = 0;
	BOOLEAN 									bMatch = FALSE;
	
	if(targetBufLen < patternBufLen) return FALSE;

	while(targetIdx < targetBufLen && patternIdx < patternBufLen)
	{
		if(targetBuf[targetIdx] == patternBuf[patternIdx])
		{
			++targetIdx;
			++patternIdx;
		}
		else
		{
			targetIdx = targetIdx + 1 - patternIdx;
			patternIdx = 0;
		}
	}
	
	if(patternIdx == patternBufLen)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("match, idx: %u\n", patternIdx));
		bMatch = TRUE;
	}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bMatch: %u\n", bMatch));
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "target:\n", targetBuf, targetBufLen);
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "pattern:\n", patternBuf, patternBufLen);

	return bMatch;
}

RT_STATUS
P2PSvc_GetDevNameFromDevInfoAttr(
	IN	 POCTET_STRING						posP2PAttrs,
	OUT POCTET_STRING						posDevName
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;

	// dev-info format:
	// 	evAddr(6), configMethod(2), primDevType(8), nSecDevTypes(1), secDevTypeList(8*n), devName TLV(2 + 2 + n)
	//

	do
	{
		OCTET_STRING						osDevInfo = {NULL, 0};
		u2Byte								offsetNSecDevTypes = 0;
		u1Byte								nSecDevTypes = 0;
		u2Byte								offsetDevName = 0;
		u2Byte								devNameLen = 0;

		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_DEVICE_INFO, 6 + 2 + 8 + 1, &osDevInfo))
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
		
		RT_PRINT_DATA(COMP_P2P, DBG_TRACE, 
			"P2PSvc_GetDevNameFromDevInfoAttr(): osDevInfo:\n", 
			osDevInfo.Octet, osDevInfo.Length);
		
		offsetNSecDevTypes = 6 + 2 + 8;

		if (osDevInfo.Octet != NULL)
		{
			nSecDevTypes = *(pu1Byte)(osDevInfo.Octet + offsetNSecDevTypes);
		}
		offsetDevName = offsetNSecDevTypes + 1 + (8 * nSecDevTypes) + 2 + 2;
		
		if(osDevInfo.Length < offsetDevName + 1) 
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(osDevInfo.Length - 6 + 2 + 8 + 1 + 8 * nSecDevTypes + 2 + 2 <= 0)
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
		
		devNameLen = osDevInfo.Length - (6 + 2 + 8 + 1 + 8 * nSecDevTypes + 2 + 2);
		
		posDevName->Octet = osDevInfo.Octet + offsetDevName;
		posDevName->Length = devNameLen;
		
	}while(FALSE);

	return rtStatus;
	
}

#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)
