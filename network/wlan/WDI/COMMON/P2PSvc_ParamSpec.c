#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)
#if (P2PSVC_SUPPORT == 1)

//
// Description:
//		This table defines the parameters of the supported actions.
//		The table is used for validating action request from upper layer.
//
P2PSVC_ACTION_SPEC_ENTRY lP2PSvcActionSpecTab[] = 
{
	{
		P2PSVC_OBJ_HDR_ID_ACT_ENABLE,
		{	
			P2PSVC_OBJ_HDR_ID_DATA_BENABLE,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},
	
	{
		P2PSVC_OBJ_HDR_ID_ACT_SEEK,
		{	
			P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME_HASH,
			P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO_REQ,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},

	{
		P2PSVC_OBJ_HDR_ID_ACT_CANCEL_SEEK,
		{	
			P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},

	{
		P2PSVC_OBJ_HDR_ID_ACT_ADVERTISE,
		{
			P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME_HASH,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_AUTO_ACCEPT,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS,
			P2PSVC_OBJ_HDR_ID_DATA_NETWORK_CONFIG,
			P2PSVC_OBJ_HDR_ID_DATA_ADV_ID,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},

	{
		P2PSVC_OBJ_HDR_ID_ACT_CANCEL_ADVERTISE,
		{
			P2PSVC_OBJ_HDR_ID_DATA_ADV_ID,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},

	{
		P2PSVC_OBJ_HDR_ID_ACT_SVC_STATUS,
		{
			P2PSVC_OBJ_HDR_ID_DATA_ADV_ID,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},

	{
		P2PSVC_OBJ_HDR_ID_ACT_PD_REQ,
		{
			P2PSVC_OBJ_HDR_ID_DATA_SESSION_ID,
			P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR,
			P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME_HASH,
			P2PSVC_OBJ_HDR_ID_DATA_ADV_ID,
			P2PSVC_OBJ_HDR_ID_DATA_SESSION_INFO, 
			P2PSVC_OBJ_HDR_ID_DATA_NETWORK_ROLE,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},

	{
		P2PSVC_OBJ_HDR_ID_ACT_FOPD_REQ,
		{
			P2PSVC_OBJ_HDR_ID_DATA_USER_ACCEPTED,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},

	{
		P2PSVC_OBJ_HDR_ID_ACT_CONN_CAP,
		{
			P2PSVC_OBJ_HDR_ID_DATA_CONN_CAP,
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},
	
	//----------------------------------------------
	{
		P2PSVC_OBJ_HDR_ID_ACT_UNKNOWN,
		{
			//-----------------------------------
			P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN // sentinel
		}
	},
};

P2PSVC_PARAM_SPEC_ENTRY lP2PSvcParamSpecTab[] = 
{
	{P2PSVC_OBJ_HDR_ID_DATA_BENABLE, 			sizeof(BOOLEAN), 	sizeof(BOOLEAN)},
	{P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 			1, 					255},
	{P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME_HASH, 	6, 					6},
	{P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO_REQ, 		0, 					65535},
	{P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO, 			0,						65535},
	{P2PSVC_OBJ_HDR_ID_DATA_SVC_AUTO_ACCEPT, 	sizeof(BOOLEAN), 	sizeof(BOOLEAN)},
	{P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS,	 		sizeof(u1Byte), 	sizeof(u1Byte)},
	{P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 				sizeof(u4Byte), 	sizeof(u4Byte)},
	{P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, 			6, 					6},
	{P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 			1, 					1},
	{P2PSVC_OBJ_HDR_ID_DATA_DEV_NAME, 			1, 					32},
	{P2PSVC_OBJ_HDR_ID_DATA_SVC_DESC_LIST, 	1, 					65535},
	{P2PSVC_OBJ_HDR_ID_DATA_SESSION_INFO, 		1, 					144},
	{P2PSVC_OBJ_HDR_ID_DATA_NETWORK_ROLE, 		sizeof(u1Byte), 	sizeof(u1Byte)},
	{P2PSVC_OBJ_HDR_ID_DATA_CONN_CAP, 			sizeof(u1Byte), 	sizeof(u1Byte)},
	{P2PSVC_OBJ_HDR_ID_DATA_SESSION_ID, 		sizeof(u4Byte), 	sizeof(u4Byte)},
	{P2PSVC_OBJ_HDR_ID_DATA_USER_ACCEPTED, 	sizeof(BOOLEAN), 	sizeof(BOOLEAN)},
	{P2PSVC_OBJ_HDR_ID_DATA_SVC_COUNT, 			sizeof(u1Byte), 	sizeof(u1Byte)},
	//--------------------------------------------------------------------------
	{P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN, 0, 0}
};

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------


PP2PSVC_PARAM_SPEC_ENTRY
P2PSvc_GetParamSpec(
	IN  u4Byte								actId
	)
{
	u4Byte 									paramSpecIdx = 0;
	PP2PSVC_PARAM_SPEC_ENTRY			pParamSpec = NULL;
	
	for(paramSpecIdx = 0; P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN != lP2PSvcParamSpecTab[paramSpecIdx].paramId; paramSpecIdx++)
	{
		if(actId == lP2PSvcParamSpecTab[paramSpecIdx].paramId)
		{
			pParamSpec = &(lP2PSvcParamSpecTab[paramSpecIdx]);
			break;
		}
	}

	return pParamSpec;

}

//
// Description:
//		Validate if the input buffer contains an object with obj list as its payload
//
RT_STATUS 
P2PSvc_ValidateReqInfo(
	IN  PVOID								infoBuf,
	IN  u4Byte								inBufLen,
	OUT pu4Byte								pBytesNeeded
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	u4Byte									bytesNeeded = 0;


	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PRT_OBJECT_HEADER					pObjHdr = NULL;
		PP2PSVC_OBJ_LIST					pObjList = NULL;
		u4Byte								objIter = 0;

		// Check if we can read the obj hdr
		bytesNeeded = RT_OBJECT_HEADER_SIZE;

		if(inBufLen < bytesNeeded)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Buffer (%d) too short for required (%d)!\n", inBufLen, bytesNeeded));
			rtStatus = RT_STATUS_INVALID_LENGTH;
			break;
		}

		// Check if the in buf is large enough to hold the data whose length is declared in the obj header
		pObjHdr = (PRT_OBJECT_HEADER)(infoBuf);
		
		if(inBufLen < bytesNeeded + pObjHdr->Length)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] in buf is not large enough to hold the value, value len: %u, inbuf len: %u!\n", pObjHdr->Length, inBufLen));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// Check if the payload is a valid obj list
		pObjList = (PP2PSVC_OBJ_LIST)(pObjHdr->Value);
		bytesNeeded += FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart);
		if(inBufLen < bytesNeeded)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] in buf is not large enough to hold an obj list, inbuf len: %u!\n", inBufLen));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// Validate the obj hdr of the info list		
		if(P2PSVC_OBJ_HDR_ID_DATA_OBJ_LIST != pObjList->hdr.Id)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] payload is not an obj list, its id is: %u\n", pObjList->hdr.Id));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// Check if the objs in the obj list has valid length
		for(objIter = 0; objIter < pObjList->nObjs; objIter++)
		{	
			PRT_OBJECT_HEADER pCurObj = NULL;
			
			// Check we can read the obj hdr of the current obj
			if(inBufLen < bytesNeeded + pObjList->ObjTab[objIter] + RT_OBJECT_HEADER_SIZE)
			{
				bytesNeeded = bytesNeeded + pObjList->ObjTab[objIter] + RT_OBJECT_HEADER_SIZE;
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] obj with index %u is not valid\n", objIter));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}

			// Get the obj
			pCurObj = P2PSVC_OBJ_LIST_GET_OBJ(pObjList, objIter);

			// Check the hdr of the cur obj
			if(inBufLen < bytesNeeded + pObjList->ObjTab[objIter] + RT_OBJECT_HEADER_SIZE + pCurObj->Length)
			{
				bytesNeeded = bytesNeeded + pObjList->ObjTab[objIter] + RT_OBJECT_HEADER_SIZE + pCurObj->Length;
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] inbuf (len: %u) is not large enough to hold the obj (index: %u, len: %u)\n", inBufLen, objIter, pCurObj->Length));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
		}

		if(RT_STATUS_SUCCESS != rtStatus)
		{
			break;
		}
	}while(FALSE);

	*pBytesNeeded = bytesNeeded;

	// Dump raw data if invalid
	if(RT_STATUS_SUCCESS != rtStatus)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] info req invalid, bytesNeeded: %u\n", *pBytesNeeded));
		RT_PRINT_DATA(COMP_P2P, DBG_WARNING, "InBuf:\n", infoBuf, inBufLen);
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}



//
// Description:
// 	Validate the object list contains all the required parameters of the id.
// 	E.g., for P2PSVC_OBJ_HDR_ID_ACT_SEEK, the obj list shall contain list of
//		[svc-name, svc-hash, svc-info-req].
// 	Each required obj shall appear just once.
//		Order is not important.
//
RT_STATUS
P2PSvc_ValidateActionParam(
	IN  u4Byte								id,
	IN  PP2PSVC_OBJ_LIST					pObjList
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	

	int										actSpecIdx = 0;
	int										nParams = 0;
	int										paramIdx = 0;
	PP2PSVC_ACTION_SPEC_ENTRY 				pActSpec = NULL;
	
	do
	{
		// Search table
		for(actSpecIdx = 0; P2PSVC_OBJ_HDR_ID_ACT_UNKNOWN != lP2PSvcActionSpecTab[actSpecIdx].actId; actSpecIdx++)
		{
			if(id == lP2PSvcActionSpecTab[actSpecIdx].actId)
			{
				pActSpec = &(lP2PSvcActionSpecTab[actSpecIdx]);
				break;
			}
		}
		
		RT_ASSERT(pActSpec, ("p2psvc_AddSeekInfo(): Spec of P2PSVC_OBJ_HDR_ID_ACT_SEEK not specified in the action spec table\n"));

		// Count # of parameters
		if (pActSpec != NULL)
		{
			for (nParams = 0; P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN != pActSpec->paramList[nParams]; nParams++) {}
		}

		// Shall have exactly 1 set of parameters
		for(paramIdx = 0; paramIdx < nParams; paramIdx++)
		{// foreach required parameter
			u4Byte					objIter = 0;
			PRT_OBJECT_HEADER		pCurObj = NULL;
			u4Byte 					nOccurrence = 0;
			
			for(objIter = 0; objIter < pObjList->nObjs; objIter++)
			{
				pCurObj = P2PSVC_OBJ_LIST_GET_OBJ(pObjList, objIter);
				if(pCurObj->Id == pActSpec->paramList[paramIdx]) nOccurrence++;
			}

			if(0 == nOccurrence)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("param (0x%08X) not found\n", pActSpec->paramList[paramIdx]));
				rtStatus = RT_STATUS_INVALID_PARAMETER;
				break;
			}
			else if(1 < nOccurrence 
				&& P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN != pActSpec->paramList[paramIdx]
				)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("id of cur obj (%u) occur more than once: %u\n", pCurObj->Id, nOccurrence));
				rtStatus = RT_STATUS_INVALID_PARAMETER;
				break;
			}
		}

		if(RT_STATUS_SUCCESS != rtStatus)
		{
			break;
		}
	}while(FALSE);

	// Dump raw data if invalid
	if(RT_STATUS_SUCCESS != rtStatus)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] info action param\n"));
		RT_PRINT_DATA(COMP_P2P, DBG_WARNING, "ObjList:\n", pObjList, P2PSVC_OBJ_LIST_LEN(pObjList));
	}

	return rtStatus;
}

//
// Description:
// 	Validate all params specified by the objcts in the obj list.
//
RT_STATUS
P2PSvc_ValidateParamContent(
	IN  PP2PSVC_OBJ_LIST					pObjList
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PRT_OBJECT_HEADER						pCurObj = NULL;
	u4Byte									objIter = 0;
	u4Byte									paramSpecIdx = 0;
	PP2PSVC_PARAM_SPEC_ENTRY			pParamSpec = NULL;
	
	do
	{
		u4Byte valLen = 0;
		
		for(objIter = 0, pParamSpec = NULL; NULL != (pCurObj = P2PSVC_OBJ_LIST_GET_OBJ(pObjList, objIter)); objIter++)
		{
			valLen = pCurObj->Length;

			if(NULL == (pParamSpec = P2PSvc_GetParamSpec(pCurObj->Id)))
			{
				//RT_TRACE(COMP_P2P, DBG_LOUD, ("%s(): param id (0x%08X) can't be found in the param spec tab\n", __FUNCTION__, pCurObj->Id));
				//rtStatus = RT_STATUS_INVALID_PARAMETER;
				//break;
				continue;
			}

			if(!(pParamSpec->minLen <= valLen && valLen <= pParamSpec->maxLen))
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("%s(): param id (0x%08X) with invalid len: %u, shall between %u and %u\n", __FUNCTION__, pCurObj->Id, valLen, pParamSpec->minLen, pParamSpec->maxLen));
				rtStatus = RT_STATUS_INVALID_LENGTH;
				break;
			}
		}
		
		if(RT_STATUS_SUCCESS != rtStatus)
		{
			break;
		}
	}while(FALSE);

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		RT_PRINT_DATA(COMP_P2P, DBG_WARNING, "ObjList:\n", pObjList, P2PSVC_OBJ_LIST_LEN(pObjList));
	}

	return rtStatus;
}

PRT_OBJECT_HEADER
P2PSvc_GetParam(
	IN  PP2PSVC_OBJ_LIST					pObjList,
	IN  u4Byte								paramId,
	IN  u4Byte								fromIdx
	)
{
	PRT_OBJECT_HEADER						pParamObj = NULL;
	PRT_OBJECT_HEADER						pCurObj = NULL;
	u4Byte									objIter = 0;

	P2PSVC_CHECK_NULL(pObjList);

	do
	{
		// Index out of range
		if(!(fromIdx < pObjList->nObjs)) break;

		// Search next appearence of the 1st param of the action
		for(objIter = fromIdx; NULL != (pCurObj = P2PSVC_OBJ_LIST_GET_OBJ(pObjList, objIter)); objIter++)
		{
			if(pCurObj->Id == paramId)
			{
				pParamObj = pCurObj;
				break;
			}
		}

		if(NULL == pParamObj)
		{// not found
			break;
		}

	}while(FALSE);

	return pParamObj;
}

RT_STATUS
P2PSvc_UpdateParam(
	IN  PP2PSVC_OBJ_LIST					pObjList,
	IN  u4Byte								paramId,
	IN  u4Byte								fromIdx,
	IN  u4Byte								bufLen,
	IN  PVOID								pBuf
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PRT_OBJECT_HEADER						pObj = NULL;

	P2PSVC_CHECK_NULL(pObjList);

	do
	{
		if(NULL == (pObj = P2PSvc_GetParam(pObjList, paramId, fromIdx)))
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(pObj->Length != bufLen)
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
		
		PlatformMoveMemory(pObj->Value, pBuf, bufLen);
		
	}while(FALSE);

	return rtStatus;
}


#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)
