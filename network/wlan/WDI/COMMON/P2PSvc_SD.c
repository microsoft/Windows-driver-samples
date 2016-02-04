#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)
#if (P2PSVC_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------


RT_STATUS
P2PSvc_ParseAnqpQueryReq(
	IN  PP2P_SERVICE_REQ_TLV		pAnqpQueryReq,
	OUT pu1Byte						pSvcNameLen,
	OUT pu1Byte						*ppSvcNameBuf,
	OUT pu1Byte						pSvcInfoReqLen,
	OUT pu1Byte						*ppSvcInfoReqBuf
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;

	u1Byte							svcNameLen = 0;
	pu1Byte							svcNameBuf = NULL;
	u1Byte							svcInfoReqLen = 0;
	pu1Byte							svcInfoReqBuf = NULL;

	pu1Byte							anqpQueryReqBuf = pAnqpQueryReq->ServiceDesc.Buffer;
	u2Byte							anqpQueryReqLen = pAnqpQueryReq->ServiceDesc.BufferLength;

	do
	{
		u2Byte						idx = 0;
		
		// svc-name-len
		if(anqpQueryReqLen < idx + 1) 					{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
		svcNameLen = *(pu1Byte)(anqpQueryReqBuf + idx);
		idx += 1;

		// svc-name
		if(anqpQueryReqLen < idx + svcNameLen) 			{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
		svcNameBuf = (pu1Byte)(anqpQueryReqBuf + idx);
		idx += svcNameLen;

		// svc-info-req-len
		if(anqpQueryReqLen < idx + 1) 					{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
		svcInfoReqLen = *(pu1Byte)(anqpQueryReqBuf + idx);
		idx += 1;
		
		// svc-info-req
		if(anqpQueryReqLen < idx + svcInfoReqLen) 		{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
		svcInfoReqBuf = (pu1Byte)(anqpQueryReqBuf + idx);
		idx += svcInfoReqLen;

		if(pSvcNameLen) *pSvcNameLen = svcNameLen;
		if(ppSvcNameBuf) *ppSvcNameBuf = svcNameBuf;
		if(pSvcInfoReqLen) *pSvcInfoReqLen = svcInfoReqLen;
		if(ppSvcInfoReqBuf) *ppSvcInfoReqBuf = svcInfoReqBuf;
	}while(FALSE);

	return rtStatus;
}

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
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;

	u1Byte							nSvcInfoDesc = 0;
	u1Byte							svcNameLen = 0;
	pu1Byte							svcNameBuf = NULL;
	u4Byte							advId = 0;
	u2Byte							configMethod = 0;
	u1Byte							svcStatus = 0;
	u2Byte							svcInfoLen = 0;
	pu1Byte							svcInfoBuf = NULL;

	pu1Byte							anqpQueryRspBuf = pAnqpQueryRsp->ServiceDesc.Buffer;
	u2Byte							anqpQueryRspLen = pAnqpQueryRsp->ServiceDesc.BufferLength;

	do
	{
		u2Byte						offset = 0;
		u2Byte						idxSvcInfoDesc = 0;

		// number of svc info desc
		if(anqpQueryRspLen < offset + 1) 						{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
		nSvcInfoDesc = *(pu1Byte)(anqpQueryRspBuf + offset);
		offset += 1;

		//DbgPrint("%s(): nSvcInfoDesc: %u, index: %u\n", __FUNCTION__, nSvcInfoDesc, index);

		if(nSvcInfoDesc <= index)
		{
			rtStatus = RT_STATUS_INVALID_PARAMETER;
			break;
		}

		for(idxSvcInfoDesc = 0; idxSvcInfoDesc < nSvcInfoDesc; idxSvcInfoDesc++)
		{
			// Addendum v0.43
			// svc-info-desc: [[adv-svc-desc], svc-status(1), svc-info-len(2), svc-info(n)]
			// where adv-svc-desc: [adv-id(4), config-method(2), svc-name-len(1), svc-name(n)]

			// adv-id
			if(anqpQueryRspLen < offset + 4) 				{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
			else 											{advId = *(pu4Byte)(anqpQueryRspBuf + offset);}
			offset += 4;

			// config-method
			if(anqpQueryRspLen < offset + 2) 				{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
			else 											{configMethod = *(pu2Byte)(anqpQueryRspBuf + offset);}
			offset += 2;
			
			// svc-name-len
			if(anqpQueryRspLen < offset + 1) 				{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
			else 											{svcNameLen = *(pu1Byte)(anqpQueryRspBuf + offset);}
			offset += 1;

			// svc-name
			if(anqpQueryRspLen < offset + svcNameLen) 		{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
			else 											{svcNameBuf = (pu1Byte)(anqpQueryRspBuf + offset);}
			offset += svcNameLen;

			// svc-status
			if(anqpQueryRspLen < offset + 1) 				{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
			else 											{svcStatus = *(pu1Byte)(anqpQueryRspBuf + offset);}
			offset += 1;

			// svc-info-len
			if(anqpQueryRspLen < offset + 2) 				{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
			else 											{svcInfoLen = *(pu2Byte)(anqpQueryRspBuf + offset);}
			offset += 2;
			
			// svc-info-req
			if(anqpQueryRspLen < offset + svcInfoLen) 		{rtStatus = RT_STATUS_INVALID_LENGTH; break;}
			else 											{svcInfoBuf = (pu1Byte)(anqpQueryRspBuf + offset);}
			offset += svcInfoLen;

			if(idxSvcInfoDesc == index)
			{
				break;
			} 
		}

		if(RT_STATUS_SUCCESS != rtStatus)
		{	
			DbgPrint("ng: 0x%08X, offset: %u\n", rtStatus, offset);
			RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ANQP Query Rsp:\n", anqpQueryRspBuf, anqpQueryRspLen);
			break;
		}

		if(pSvcNameLen) 		*pSvcNameLen = svcNameLen;
		if(ppSvcNameBuf) 		*ppSvcNameBuf = svcNameBuf;
		if(pAdvId) 				*pAdvId = advId;
		if(pSvcStatus) 			*pSvcStatus = svcStatus;
		if(pSvcInfoLen) 		*pSvcInfoLen = svcInfoLen;
		if(ppSvcInfoBuf) 		*ppSvcInfoBuf = svcInfoBuf;
	}while(FALSE);

	return rtStatus;
}


RT_STATUS
P2PSvc_SDReq(
	IN	 PP2PSVC_INFO						pP2PSvcInfo,
	IN	 pu1Byte								devAddr,
	IN  PP2PSVC_REQ_INFO_ENTRY			pSeekInfoEntry,
	IN  PRT_OBJECT_HEADER				pSvcInfoReqObj,
	IN  u1Byte								searchId
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO								pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	PP2P_SD_CONTEXT						pSDContext = &(pP2PInfo->SDContext);
	PRT_OBJECT_HEADER 					pSvcNameObj = P2PSvc_GetParam(&pSeekInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 0);
	u2Byte									bytesWritten = 0;

	P2PSVC_FUNC_IN(DBG_LOUD);
	
	cpMacAddr(pSDContext->UserSDReq.TargetDeviceAddress, devAddr);
	cpMacAddr(pSDContext->TargetDeviceAddress, devAddr);
	
	pSDContext->UserSDReq.ServiceReqTLVSize = 1;

	pSDContext->UserSDReq.ServiceReqTLVList[0].TransactionID = searchId;
	pSDContext->UserSDReq.ServiceReqTLVList[0].ServiceDesc.ServiceType = P2P_SD_PROTOCOL_P2PSVC;

	// svc-name-len
	pSDContext->UserSDReq.ServiceReqTLVList[0].ServiceDesc.Buffer[bytesWritten] = (u1Byte)pSvcNameObj->Length;
	bytesWritten += 1;

	// svc-name
	PlatformMoveMemory(pSDContext->UserSDReq.ServiceReqTLVList[0].ServiceDesc.Buffer + bytesWritten, 
		pSvcNameObj->Value, 
		pSvcNameObj->Length);
	bytesWritten += (u2Byte)pSvcNameObj->Length;

	// svc-info-req-len
	pSDContext->UserSDReq.ServiceReqTLVList[0].ServiceDesc.Buffer[bytesWritten] = (u1Byte)pSvcInfoReqObj->Length;
	bytesWritten += 1;

	// svc-info-req
	PlatformMoveMemory(pSDContext->UserSDReq.ServiceReqTLVList[0].ServiceDesc.Buffer + bytesWritten, 
		pSvcInfoReqObj->Value, 
		pSvcInfoReqObj->Length);
	bytesWritten += (u2Byte)pSvcInfoReqObj->Length;

	pSDContext->UserSDReq.ServiceReqTLVList[0].ServiceDesc.BufferLength = bytesWritten;
	
	pSDContext->bRequester = TRUE;
	pSDContext->DialogToken = IncreaseDialogToken(pP2PInfo->DialogToken);
	//pSDContext->TransactionID = 0xAA; // seems this is for parsing recvd SDRsp
	pSDContext->FragmentID = 0;
	pSDContext->ANQPQueryRspFieldToSendOffset = 0;

	p2p_Send_SDReq(pP2PInfo, devAddr);

	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Send P2PSvc SD req to:\n", devAddr);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;

}

RT_STATUS
P2PSvc_SDRsp(
	IN	 PP2PSVC_INFO						pP2PSvcInfo,
	IN	 pu1Byte								devAddr,
	IN  u1Byte								dlgToken,
	IN  u1Byte								transactionId,
	IN  u1Byte								nSvcInfoDesc,
	IN  FRAME_BUF							*pSvcInfoDescBuf
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO								pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	PP2P_SD_CONTEXT						pSDContext = &(pP2PInfo->SDContext);


	P2P_SD_RSP_CONTEXT					sdRspCtx;

	// Fill a SD rsp context
	PlatformFillMemory(&sdRspCtx, sizeof(sdRspCtx), 0);
	sdRspCtx.SDStatus = P2P_SD_STATUS_SUCCESS;
	PlatformMoveMemory(sdRspCtx.SourceDeviceAddress, pP2PInfo->DeviceAddress, 6);
	sdRspCtx.ServiceUpdateIndicator = 0;
	sdRspCtx.ServiceRspTLVSize = 1;
	sdRspCtx.ServiceRspTLVList[0].TransactionID = transactionId;
	sdRspCtx.ServiceRspTLVList[0].Status = P2P_SD_STATUS_SUCCESS;
	sdRspCtx.ServiceRspTLVList[0].ServiceDesc.ServiceType = P2P_SD_PROTOCOL_P2PSVC;
	sdRspCtx.ServiceRspTLVList[0].ServiceDesc.BufferLength = 1 + FrameBuf_Length(pSvcInfoDescBuf);
	sdRspCtx.ServiceRspTLVList[0].ServiceDesc.Buffer[0] = nSvcInfoDesc;
	PlatformMoveMemory(sdRspCtx.ServiceRspTLVList[0].ServiceDesc.Buffer + 1, FrameBuf_MHead(pSvcInfoDescBuf), FrameBuf_Length(pSvcInfoDescBuf));

	//RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "SD Rsp data:\n", sdRspCtx.ServiceRspTLVList[0].ServiceDesc.Buffer, sdRspCtx.ServiceRspTLVList[0].ServiceDesc.BufferLength);

	p2p_Construct_AnqpQueryRspField(pP2PInfo, &sdRspCtx, pSDContext);

	// To send from offset 0
	pSDContext->ANQPQueryRspFieldToSendOffset = 0;

	p2p_Send_SDRsp(pP2PInfo, devAddr, dlgToken, P2P_SD_STATUS_SUCCESS, FALSE);

	return rtStatus;

}

BOOLEAN
P2PSvc_SDMatchSvcName(
	IN  PP2PSVC_REQ_INFO_ENTRY			pAdvInfoEntry,
	IN  u1Byte								svcNameLen,
	IN  pu1Byte								svcNameBuf
	)
{
	PRT_OBJECT_HEADER 					pSvcNameObj = NULL;
	BOOLEAN 									bMatch = FALSE;

	do
	{
		if(NULL == (pSvcNameObj = P2PSvc_GetParam(&pAdvInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 0)))
		{
			break;
		}

		// Check if it is prefix search
		if('*' == svcNameBuf[svcNameLen - 1])
		{// ended with *, assume UTF-8 encode of * uses only a single byte
			if(svcNameLen - 1 <= (u1Byte)pSvcNameObj->Length)
			{// the svcName minus the * is no longer the the advertised svc name
				if(0 == PlatformCompareMemory(svcNameBuf, pSvcNameObj->Value, svcNameLen - 1))
				{
					bMatch = TRUE;
					break;
				}
			}
		}

		if(svcNameLen != pSvcNameObj->Length) break;

		if(0 != PlatformCompareMemory(svcNameBuf, pSvcNameObj->Value, pSvcNameObj->Length)) break;

		bMatch = TRUE;

	}while(FALSE);

	return bMatch;
}


BOOLEAN
P2PSvc_ToProcessSDRsp(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN	 u1Byte								searchId,
	IN  pu1Byte								devAddr
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_SR_LIST_ENTRY 				pSREntryInList = NULL;
	BOOLEAN									bAccept = FALSE;

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_GetSearchResult(pP2PSvcInfo, searchId, devAddr, &pSREntryInList)))
		{
			break;
		}

		P2PSVC_CHECK_NULL(pSREntryInList);

		if(NULL == pSREntryInList)
		{
			bAccept = FALSE;
			break;
		}
		else
		{// already have an entry in search result list
			u8Byte							curTime = PlatformGetCurrentTime();

			if(0 == pSREntryInList->sdRspRxTime)
			{
				bAccept = TRUE;
				break;
			}

			if(0 != pSREntryInList->sdRspRxTime && P2PSVC_LEAST_SD_RSP_TIME_uS < curTime - pSREntryInList->sdRspRxTime)
			{// we've received a SD rsp but it is a long time ago
				bAccept = TRUE;
				break;
			}
		}
	}while(FALSE);

	if(!bAccept)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject SD rsp\n"));
	}

	return bAccept;
}

#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)