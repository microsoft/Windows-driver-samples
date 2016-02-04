#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)
#if (P2PSVC_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
u4Byte
p2psvc_AttachObjToObjList(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						type,
	IN  u4Byte 						id,
	IN  u4Byte						ver,
	IN  u4Byte						len,
	IN  PVOID						buf
	)
{
	// Assume that
	// 		* the obj list is valid
	// 		* the object to attach is valid
	//

	ULONG 							bytesWritten = 0;
	PRT_OBJECT_HEADER				pObjToFill = NULL;
	
	if(0 == pObjList->nObjs)
	{
		pObjList->ObjTab[0] = 0;
		pObjToFill = (PRT_OBJECT_HEADER)(pObjList->varStart + pObjList->ObjTab[0]);
	}
	else
	{
		PRT_OBJECT_HEADER 			pPrevObj = (PRT_OBJECT_HEADER)(pObjList->varStart + pObjList->ObjTab[pObjList->nObjs - 1]);
		u4Byte 						prevObjLen = RT_OBJECT_HEADER_SIZE + pPrevObj->Length;

		pObjList->ObjTab[pObjList->nObjs] = pObjList->ObjTab[pObjList->nObjs - 1] + prevObjLen;
		pObjToFill = (PRT_OBJECT_HEADER)(pObjList->varStart + pObjList->ObjTab[pObjList->nObjs]);
	}

	// Fill the obj
	RT_ASSIGN_OBJECT_HEADER(pObjToFill, 
		type, 
		id, 
		ver, 
		len);

	if(0 < len)
	{
		PlatformMoveMemory(pObjToFill->Value, buf, len);
	}
	
	bytesWritten = RT_OBJECT_HEADER_SIZE + len;

	// Update obj list header
	pObjList->nObjs++;
	pObjList->hdr.Length += bytesWritten;

	return bytesWritten;
}


//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

u4Byte
P2PSvc_MakeObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						id,
	IN  u4Byte						bufLEn,
	IN  PVOID						pBuf
	)
{
	u4Byte							bytesWritten = 0;

	do
	{	
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					id, 
					P2PSVC_MIN_SUPPORT_VER, 
					bufLEn, 
					pBuf);
	}while(FALSE);
	
	return bytesWritten;	
}

u4Byte
P2PSvc_MakeSvcDescObjList(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	OUT PP2PSVC_OBJ_LIST			*ppSvcDescObjList
	)
{
	u4Byte							bytesWritten = 0;
	P2PSVC_OBJ_LIST					objList;
	
	do
	{
		PlatformZeroMemory(&objList, sizeof(objList));
		P2PSVC_OBJ_LIST_INIT(&objList, 
			RT_OB_HDR_TYPE_DATA, 
			P2PSVC_OBJ_HDR_ID_DATA_SVC_DESC_LIST, 
			P2PSVC_MIN_SUPPORT_VER);

		// p2psvc_AttachObjToObjList will make obj hdr for us,
		// so we copy from the fild right after the obj hdr.
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SVC_DESC_LIST, 
					P2PSVC_MIN_SUPPORT_VER, 
					FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart) - FIELD_OFFSET(RT_OBJECT_HEADER, Value), 
					(pu1Byte)(&objList) + FIELD_OFFSET(RT_OBJECT_HEADER, Value));

		if(ppSvcDescObjList)
		{
			*ppSvcDescObjList = (PP2PSVC_OBJ_LIST)(pObjList->varStart + pObjList->ObjTab[pObjList->nObjs - 1]);
		}
	}while(FALSE);
	
	return bytesWritten;
}

//
// Description:
//		Currently not used
//
u4Byte
P2PSvc_MakeTimeObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						paramId,
	IN  u8Byte				 		time
	)
{
	u4Byte							bytesWritten = 0;

	do
	{	
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					paramId, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(time), 
					&time);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeSearchIdObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  PP2PSVC_REQ_INFO_ENTRY 		pSeekInfoEntry,
	OUT pu1Byte						pSearchId
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		PRT_OBJECT_HEADER 			pSearchIdObj = NULL;

		if(NULL == (pSearchIdObj = P2PSvc_GetParam(&pSeekInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 0)))
		{
			break;
		}

		if(1 != pSearchIdObj->Length)
		{
			break;
		}

		if(pSearchId) *pSearchId = *((pu1Byte)pSearchIdObj->Value);
		
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 
					P2PSVC_MIN_SUPPORT_VER, 
					pSearchIdObj->Length, 
					pSearchIdObj->Value);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeDevAddrObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  pu1Byte						pDevAddr
	)
{
	u4Byte							bytesWritten = 0;
	u1Byte							bcstAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	bytesWritten = p2psvc_AttachObjToObjList(pObjList,
						RT_OB_HDR_TYPE_DATA,
						P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR,
						P2PSVC_MIN_SUPPORT_VER, 
						6, 
						(NULL == pDevAddr) ? (bcstAddr) : (pDevAddr));
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeDevNameObj_FromP2PAttrs(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  POCTET_STRING				posP2PAttrs
	)
{
	u4Byte							bytesWritten = 0;
	OCTET_STRING 					osDevName = {NULL, 0};

	do
	{
		if(RT_STATUS_SUCCESS != P2PSvc_GetDevNameFromDevInfoAttr(posP2PAttrs, &osDevName))
		{
			break;
		}
		
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
							RT_OB_HDR_TYPE_DATA,
							P2PSVC_OBJ_HDR_ID_DATA_DEV_NAME,
							P2PSVC_MIN_SUPPORT_VER, 
							osDevName.Length,
							osDevName.Octet);

		RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2PSvc_MakeDevNameObj_FromP2PAttrs():\n", osDevName.Octet, osDevName.Length);
	}while(FALSE);

	return bytesWritten;
}

u4Byte
P2PSvc_MakeDevNameObj_FromDevNameOctet(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  OCTET_STRING				osDevName
	)
{
	u4Byte							bytesWritten = 0;

	do
	{		
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
							RT_OB_HDR_TYPE_DATA,
							P2PSVC_OBJ_HDR_ID_DATA_DEV_NAME,
							P2PSVC_MIN_SUPPORT_VER, 
							osDevName.Length,
							osDevName.Octet);
	}while(FALSE);

	return bytesWritten;
}


u4Byte
P2PSvc_MakeAdvIdObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						advId
	)
{
	u4Byte							bytesWritten = 0;

	bytesWritten = p2psvc_AttachObjToObjList(pObjList,
						RT_OB_HDR_TYPE_DATA,
						P2PSVC_OBJ_HDR_ID_DATA_ADV_ID,
						P2PSVC_MIN_SUPPORT_VER, 
						sizeof(advId), 
						&advId);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeSessionIdObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						sessionId
	)
{
	u4Byte							bytesWritten = 0;

	bytesWritten = p2psvc_AttachObjToObjList(pObjList,
						RT_OB_HDR_TYPE_DATA,
						P2PSVC_OBJ_HDR_ID_DATA_SESSION_ID,
						P2PSVC_MIN_SUPPORT_VER, 
						sizeof(sessionId), 
						&sessionId);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeSvcNameObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte						svcNameBufLen,
	IN  pu1Byte						pSvcNameBuf
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		// The svc-name shall be encoded in UTF-8 and
		// it is upper's responsibility to translate
		// it to other format (e.g., UTF-16).

		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 
					P2PSVC_MIN_SUPPORT_VER, 
					svcNameBufLen, 
					pSvcNameBuf);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeSvcInfoObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u2Byte						svcInfoBufLen,
	IN  pu1Byte						pSvcInfoBuf
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO, 
					P2PSVC_MIN_SUPPORT_VER, 
					svcInfoBufLen, 
					pSvcInfoBuf);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeAutoAcceptObj(
	IN  PP2PSVC_OBJ_LIST 			pObjList,
	IN  BOOLEAN						bAutoAccept
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SVC_AUTO_ACCEPT, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(bAutoAccept), 
					&bAutoAccept);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeSvcStatusObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte						svcStatus
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS, 
					P2PSVC_MIN_SUPPORT_VER, 
					1, 
					&svcStatus);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeUserAcceptedObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  BOOLEAN						bUserAccepted
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_USER_ACCEPTED, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(bUserAccepted), 
					&bUserAccepted);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeSvcCountObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte						svcCount
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SVC_COUNT, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(svcCount), 
					&svcCount);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeP2PStatusObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  P2P_STATUS_CODE 			P2PStatus
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_P2P_STATUS, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(P2PStatus), 
					&P2PStatus);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeSsidObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte 						ssidLen,
	IN  pu1Byte						ssidBuf
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SSID, 
					P2PSVC_MIN_SUPPORT_VER, 
					ssidLen, 
					ssidBuf);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeIntfAddrObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  pu1Byte						intfAddr
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_INTF_ADDR, 
					P2PSVC_MIN_SUPPORT_VER, 
					6, 
					intfAddr);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeOpChannelObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte		 				opChannel
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_OP_CHNL, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(opChannel), 
					&opChannel);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeConfigMethodObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u2Byte 						configMethod
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_CONFIG_METHOD, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(configMethod), 
					&configMethod);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeP2PAttrsObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u2Byte						attrLen,
	IN  pu1Byte						pAttrs
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_P2P_ATTRIBUTES, 
					P2PSVC_MIN_SUPPORT_VER, 
					attrLen, 
					pAttrs);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeDeferredObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  BOOLEAN						bDeferred
	)
{
	u4Byte							bytesWritten = 0;

	bytesWritten = p2psvc_AttachObjToObjList(pObjList,
						RT_OB_HDR_TYPE_DATA,
						P2PSVC_OBJ_HDR_ID_DATA_DEFERRED,
						P2PSVC_MIN_SUPPORT_VER, 
						sizeof(bDeferred), 
						&bDeferred);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeIsSDDoneObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  BOOLEAN						bSDDone
	)
{
	u4Byte							bytesWritten = 0;

	bytesWritten = p2psvc_AttachObjToObjList(pObjList,
						RT_OB_HDR_TYPE_DATA,
						P2PSVC_OBJ_HDR_ID_DATA_IS_SD_DONE,
						P2PSVC_MIN_SUPPORT_VER, 
						sizeof(bSDDone), 
						&bSDDone);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeIsPersistentObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  BOOLEAN						bPersistent
	)
{
	u4Byte							bytesWritten = 0;

	bytesWritten = p2psvc_AttachObjToObjList(pObjList,
						RT_OB_HDR_TYPE_DATA,
						P2PSVC_OBJ_HDR_ID_DATA_IS_PERSISTENT,
						P2PSVC_MIN_SUPPORT_VER, 
						sizeof(bPersistent), 
						&bPersistent);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeConnActionObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte 						connAction
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_CONN_ACTION, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(connAction), 
					&connAction);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeReasonObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  P2PSVC_REASON 				reason	
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_REASON, 
					P2PSVC_MIN_SUPPORT_VER, 
					sizeof(reason), 
					&reason);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeSessionInfoDataInfoObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  OCTET_STRING				osSessionInfoDataInfo
	)
{
	u4Byte							bytesWritten = 0;

	do
	{
		bytesWritten = p2psvc_AttachObjToObjList(pObjList, 
					RT_OB_HDR_TYPE_DATA,
					P2PSVC_OBJ_HDR_ID_DATA_SESSION_INFO, 
					P2PSVC_MIN_SUPPORT_VER, 
					osSessionInfoDataInfo.Length, 
					osSessionInfoDataInfo.Octet);
	}while(FALSE);
	
	return bytesWritten;
}

u4Byte
P2PSvc_MakeGrpDevAddrObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  pu1Byte						pGrpDevAddr
	)
{
	u4Byte							bytesWritten = 0;

	bytesWritten = p2psvc_AttachObjToObjList(pObjList,
						RT_OB_HDR_TYPE_DATA,
						P2PSVC_OBJ_HDR_ID_DATA_GRP_DEV_ADDR,
						P2PSVC_MIN_SUPPORT_VER, 
						6, 
						pGrpDevAddr);
	
	return bytesWritten;
}

#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)