#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)
#if (P2PSVC_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

BOOLEAN
p2psvc_MatchSvcNameHash_org_wi_fi_wfds(
	IN  POCTET_STRING				posSvcNameHash
)
{
	BOOLEAN bMatched = FALSE;

	u4Byte	nHashes = posSvcNameHash->Length / 6;
	u4Byte 	iterReqHash = 0;

	for(iterReqHash = 0; iterReqHash < nHashes; iterReqHash++)
	{
		if(0 == PlatformCompareMemory(posSvcNameHash->Octet + (6 * iterReqHash), gP2PSVC_NameHashTab[0].hash, 6))
		{// check if it is prefix search
			bMatched = TRUE;
			break;
		}
	}
	
	return bMatched;
}

BOOLEAN
p2psvc_MatchSvcNameHash(
	IN  PP2PSVC_REQ_INFO_ENTRY		pInfoEntry,
	IN  POCTET_STRING				posSvcNameHash
	)
{		
	BOOLEAN							bMatch = FALSE;
	PRT_OBJECT_HEADER				pSvcNameHashObj = NULL;
	u4Byte							nHashes = posSvcNameHash->Length / 6;
	u4Byte 							iterReqHash = 0;

	if(NULL == (pSvcNameHashObj = P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME_HASH, 0)))
	{// shall not happen since we have validated when set this req
		return FALSE;
	}

	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "adv svc hash:\n", pSvcNameHashObj->Value, pSvcNameHashObj->Length);
	
	for(iterReqHash = 0; iterReqHash < nHashes; iterReqHash++)
	{
		if(0 == PlatformCompareMemory(posSvcNameHash->Octet + (6 * iterReqHash), pSvcNameHashObj->Value, 6))
		{
			RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "Match svc-name-hash:\n", pSvcNameHashObj->Value, pSvcNameHashObj->Length);
			bMatch = TRUE;
			break;
		}
	}

	return bMatch;
}

u2Byte
p2psvc_EvalSvcAdvInfoAttrLen(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  POCTET_STRING				posSvcNameHash
	)
{
	u2Byte 							reqLen = 0;
	u1Byte							nSvcToAttach = 0;

	do
	{
		PRT_LIST_ENTRY 				pEntry = NULL;
		
		if(p2psvc_MatchSvcNameHash_org_wi_fi_wfds(posSvcNameHash))
		{// Spec 3.4.3.2: if prefix search, send org.wi-fi.wfds service name
		
			reqLen += (u2Byte)(	sizeof(u4Byte)								// adv-id
								+ 2											// config-method
								+ sizeof(u1Byte)							// svc-name len
								+ strlen(P2PSVC_WIFI_WFDS_SERVICE_NAME)		// svc-name
								+ 0);

			nSvcToAttach++;
		}
		else
		{
		  for(pEntry = RTGetHeadList(&pP2PSvcInfo->advSvcList); pEntry != &pP2PSvcInfo->advSvcList; pEntry = RTNextEntryList(pEntry))
		  {
			  PP2PSVC_REQ_INFO_ENTRY	pInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
			  PRT_OBJECT_HEADER 		pAdvIdObj = NULL;
			  PRT_OBJECT_HEADER 		pSvcNameObj = NULL;
  
			  if(!p2psvc_MatchSvcNameHash(pInfoEntry, posSvcNameHash)) continue;
			  
			  pAdvIdObj 	= P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
			  pSvcNameObj = P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 0);
  
			  reqLen += (u2Byte)(pAdvIdObj->Length							// adv-id
			  						+ 2										// config-method
									+ sizeof(u1Byte)						// svc-name len
									+ pSvcNameObj->Length					// svc-name
									+ 0);
  
			  nSvcToAttach++;
		  }
		}
	}while(FALSE);

	if(0 == nSvcToAttach) reqLen = 0;

	return reqLen;
}

RT_STATUS
p2psvc_MakePDSvcNameHash(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	OUT FRAME_BUF					*pBuf
	) 
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	pu1Byte							pLen = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PRT_LIST_ENTRY 				pEntry = NULL;

		PP2PSVC_PD_ENTRY			pPDEntry = NULL;
		PRT_OBJECT_HEADER			pSvcNameHashObj = NULL;

		if(!P2PSvc_PDSessionInProgress(pP2PSvcInfo, &pPDEntry))
		{
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		if(!pPDEntry->bInitor)
		{
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		if(NULL == (pSvcNameHashObj = P2PSvc_GetParam(P2PSvc_Get_PDEntryObjList(pPDEntry), P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME_HASH, 0))
				|| 6 != pSvcNameHashObj->Length
			)
		{// shall not happen since we have validated when set this req
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(NULL == (pLen = p2p_add_IEHdr(pBuf))) break;

		FrameBuf_Add_u1(pBuf, P2P_ATTR_SVC_HASH);
		FrameBuf_Add_le_u2(pBuf, (u2Byte)pSvcNameHashObj->Length);
		FrameBuf_Add_Data(pBuf, pSvcNameHashObj->Value, (u2Byte)pSvcNameHashObj->Length);

		p2p_update_IeHdrLen(pBuf, pLen);
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}


RT_STATUS
p2psvc_MakeSeekSvcNameHash(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	OUT FRAME_BUF					*pBuf
	) 
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	pu1Byte							pLen = NULL;

	P2PSVC_FUNC_IN(DBG_TRACE);

	do
	{
		PRT_LIST_ENTRY 				pEntry = NULL;
		pu2Byte						pAttrLen = NULL;

		if(NULL == (pLen = p2p_add_IEHdr(pBuf))) break;

		FrameBuf_Add_u1(pBuf, P2P_ATTR_SVC_HASH);
		FrameBuf_Add_le_u2(pBuf, (u2Byte) pP2PSvcInfo->seekReqListCnt * 6);

		// Construct the hash list
		for(pEntry = RTGetHeadList(&pP2PSvcInfo->seekReqList); pEntry != &pP2PSvcInfo->seekReqList; pEntry = RTNextEntryList(pEntry))
		{
			PP2PSVC_REQ_INFO_ENTRY	pInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
			PRT_OBJECT_HEADER 		pSvcNameHashObj = NULL;
			
			pSvcNameHashObj = P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME_HASH, 0);
			FrameBuf_Add_Data(pBuf, pSvcNameHashObj->Value, 6);
		}

		p2p_update_IeHdrLen(pBuf, pLen);
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_TRACE, rtStatus);

	return rtStatus;
}


//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------



//
// Description:
// 	Make adv-svc-info attr. 
//		The attr is carried in a independent P2P IE and if the IE is too big
//		to fit in a single P2P IE, it is splitted into several.
// 	If error occurs during the process, length of the posMsdu member 
//		will not be updated therefore no change is made to the original
//		constructing frame.
//
RT_STATUS
P2PSvc_MakeAdvSvcInfo(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  POCTET_STRING				posSvcNameHash,
	OUT FRAME_BUF					*pBuf
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO						pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	pu1Byte							pLen = NULL;

	P2PSVC_FUNC_IN(DBG_TRACE);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PRT_LIST_ENTRY 				pEntry = NULL;
		u2Byte						attrLen = 0;
		u2Byte						bytesWritten = 0;

		if(0 == (attrLen = p2psvc_EvalSvcAdvInfoAttrLen(pP2PSvcInfo, posSvcNameHash)))
		{// no matching svc
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(0 == attrLen) break;

		if(NULL == (pLen = p2p_add_IEHdr(pBuf))) break;
		
		// Attr id and len
		FrameBuf_Add_u1(pBuf, P2P_ATTR_ADV_SVC_INFO);
		FrameBuf_Add_le_u2(pBuf, attrLen);
		
		// advertise-svc-desc
		if(p2psvc_MatchSvcNameHash_org_wi_fi_wfds(posSvcNameHash))
		{// Spec 3.4.3.2: if prefix search, send org.wi-fi.wfds service name

			u4Byte 					AdvIdZero = 0;

			// Main spec v0.43:
			// If ASP finds a match with the Service Hash value of 
			// "org.wi-fi.wfds" (i.e. advertises one of the Wi-Fi alliance 
			// defined services listed in 3.2), then the ConfigMethods value 
			// in Probe Response shall be ignored. 
			
			// Attach [adv-id, config-method, svc-name len, svc-name]
			FrameBuf_Add_le_u4(pBuf, AdvIdZero);
			FrameBuf_Add_le_u2(pBuf, 0);
			FrameBuf_Add_u1(pBuf, (u1Byte)(strlen(P2PSVC_WIFI_WFDS_SERVICE_NAME)));
			FrameBuf_Add_Data(pBuf, P2PSVC_WIFI_WFDS_SERVICE_NAME, (u2Byte) strlen(P2PSVC_WIFI_WFDS_SERVICE_NAME));			
		}
		else
		{
			// Attr payload
			for(pEntry = RTGetHeadList(&pP2PSvcInfo->advSvcList); pEntry != &pP2PSvcInfo->advSvcList; pEntry = RTNextEntryList(pEntry))
			{
				PP2PSVC_REQ_INFO_ENTRY		pInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
				u2Byte						svcConfigMethod = pP2PInfo->WpsAttributes.ConfigMethod;

				PRT_OBJECT_HEADER			pNetworkConfigObj = NULL;
				PRT_OBJECT_HEADER 			pAdvIdObj = NULL;
				PRT_OBJECT_HEADER 			pSvcNameObj = NULL;

				if(!p2psvc_MatchSvcNameHash(pInfoEntry, posSvcNameHash)) continue;

				pAdvIdObj = P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
				pSvcNameObj = P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 0);
				pNetworkConfigObj = P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_NETWORK_CONFIG, 0);

				if(2 == *(PUCHAR)pNetworkConfigObj->Value)
				{// WSC PIN method only
					CLEAR_FLAG(svcConfigMethod, P2P_WPS_CONFIG_METHODS_SVC_DEFAULT_PIN);
				}

				// Attach [adv-id, config-method, svc-name len, svc-name]
				FrameBuf_Add_le_u4(pBuf, *(pu4Byte)pAdvIdObj->Value);
				FrameBuf_Add_be_u2(pBuf, svcConfigMethod);
				FrameBuf_Add_u1(pBuf, (u1Byte)pSvcNameObj->Length);
				FrameBuf_Add_Data(pBuf, pSvcNameObj->Value, (u2Byte)pSvcNameObj->Length);

				//RT_PRINT_STR(COMP_P2P, DBG_TRACE, "Advertise svc-name:\n", pSvcNameObj->Value, pSvcNameObj->Length);
			}
		}

		p2p_update_IeHdrLen(pBuf, pLen);
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_TRACE, rtStatus);

	return rtStatus;
}

//
// Description:
//		Build independent P2P IEs for carrying the hash values
//
RT_STATUS
P2PSvc_MakeSvcNameHash(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	OUT FRAME_BUF					*pBuf
	) 
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_TRACE);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		if(pP2PSvcInfo->pdSessionListCnt)
		{
			if(RT_STATUS_SUCCESS != (rtStatus = p2psvc_MakePDSvcNameHash(pP2PSvcInfo, pBuf)))
			{
				break;
			}
		}
		else if(pP2PSvcInfo->seekReqListCnt)
		{
			if(RT_STATUS_SUCCESS != (rtStatus = p2psvc_MakeSeekSvcNameHash(pP2PSvcInfo, pBuf)))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_TRACE, rtStatus);

	return rtStatus;
}


RT_STATUS
P2PSvc_MakeInitorPDReqIE(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	OUT FRAME_BUF					*pBuf
	) 
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO						pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	PP2PSVC_INITOR_PD_ENTRY			pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;
	pu1Byte							pLen = NULL;

	do
	{
		PRT_OBJECT_HEADER 			pObj = NULL;
		pu1Byte						peerDevAddr = NULL;
		u4Byte						advId = 0;
		pu1Byte						selfDevAddr = NULL;
		u4Byte						sessionId = 0;

		if(NULL == (pLen = p2p_add_IEHdr(pBuf))) break;

		// Comment out because already appended by P2PConstructProvisionDiscoveryReq
		// P2P-cap, always present
		//bytesCopied = P2PMakeP2PCapability(pP2PInfo, Type_Action, osAttr.Octet + osAttr.Length);
		//osAttr.Length += bytesCopied;

		// Comment out because already appended by P2PConstructProvisionDiscoveryReq
		// P2P-dev-info, always present
		//bytesCopied = P2PMakeP2PDeviceInfo(pP2PInfo, Type_Action, osAttr.Octet + osAttr.Length);
		//osAttr.Length += bytesCopied;

		// Group-id, contains NEW or GO
		if(TEST_FLAG(pPDEntry->selfConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_GROUP_ID);
			FrameBuf_Add_le_u2(pBuf, 6 + pPDEntry->selfGrpInfo.ssidLen);
			FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.grpDevAddr, 6);
			FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.ssidBuf, pPDEntry->selfGrpInfo.ssidLen);
		}
		
		// Intended-intf-addr, contains NEW or GO
		if(TEST_FLAG(pPDEntry->selfConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_INTENDED_INTERFACE_ADDRESS);
			FrameBuf_Add_le_u2(pBuf, 6);
			FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.grpIntfAddr, 6);
		}

		// Status, NONE

		// Operating-channel, contains NEW or GO
		if(TEST_FLAG(pPDEntry->selfConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
		{
			P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->OperatingChannel);
		}

		// Channel-list, contains NEW
		if(TEST_FLAG(pPDEntry->selfConnCap, P2PSVC_CONN_CAP_BMP_NEW))
		{
			P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pP2PInfo->ChannelEntryList);
		}

		// session-info-data-info (n <= 144), present if session-info is not 0
		pObj = P2PSvc_GetParam(&pInitorPDEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SESSION_INFO, 0);
		if(0 < pObj->Length && pObj->Length < 144)
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_SESSION_INFO_DATA_INFO);
			FrameBuf_Add_le_u2(pBuf, (u2Byte)pObj->Length);
			FrameBuf_Add_Data(pBuf, pObj->Value, (u2Byte)pObj->Length);
		}

		// conn-cap, always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_CONN_CAP_INFO);
		FrameBuf_Add_le_u2(pBuf, 1);
		FrameBuf_Add_u1(pBuf, pPDEntry->selfConnCap);

		// adv-id (adv-id, svc-mac), always present
		pObj = P2PSvc_GetParam(&pInitorPDEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, 0);
		peerDevAddr = pObj->Value;
		pObj = P2PSvc_GetParam(&pInitorPDEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
		advId = *(pu4Byte)pObj->Value;

		FrameBuf_Add_u1(pBuf, P2P_ATTR_ADV_ID_INFO);
		FrameBuf_Add_le_u2(pBuf, 4 + 6);
		FrameBuf_Add_le_u4(pBuf, advId);
		FrameBuf_Add_Data(pBuf, peerDevAddr, 6);

		// Config-timeout, contains NEW or GO
		if(TEST_FLAG(pPDEntry->selfConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
		{	
			P2PAttr_Make_ConfigTimeout(pBuf, pP2PInfo->GOConfigurationTimeout, pP2PInfo->ClientConfigurationTimeout);
		}

		// Listen-channel, contains NEW or CLI
		if(TEST_FLAG(pPDEntry->selfConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_CLI))
		{
			P2PAttr_Make_ListenChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->ListenChannel);
		}
		
		// session-id (session-id, self dev-addr), always present
		selfDevAddr = pP2PInfo->DeviceAddress;
		pObj = P2PSvc_GetParam(&pInitorPDEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SESSION_ID, 0);
		sessionId = *(pu4Byte)pObj->Value;

		FrameBuf_Add_u1(pBuf, P2P_ATTR_SESSION_ID_INFO);
		FrameBuf_Add_le_u2(pBuf, 4 + 6);
		FrameBuf_Add_le_u4(pBuf, sessionId);
		FrameBuf_Add_Data(pBuf, selfDevAddr, 6);

		// Feature-cap, always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_FEATURE_CAP_INFO);
		FrameBuf_Add_le_u2(pBuf, 2);
		FrameBuf_Add_le_u2(pBuf, 0x0001);

		// Persistent-grp-info, present if persistent grp available and ASP intent using it
		// TODO:

		//4 Indicate P2P attrs sent
		P2PSvc_IndicateFrameSent(pP2PSvcInfo, 
			P2PSVC_OBJ_HDR_ID_INDIC_INITOR_PD_REQ_SENT, 
			peerDevAddr,
			(u2Byte)(FrameBuf_Tail(pBuf) - 1 - pLen), 
			pLen + 1);

		p2p_update_IeHdrLen(pBuf, pLen);
	}while(FALSE);

	return rtStatus;
}

RT_STATUS
P2PSvc_MakeRspdorFOPDReqIE(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	OUT FRAME_BUF					*pBuf
	) 
{
		RT_STATUS					rtStatus = RT_STATUS_SUCCESS;
		PP2P_INFO					pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
		pu1Byte						pLen = NULL;

		P2PSVC_FUNC_IN(DBG_LOUD);

		do
		{
			PP2PSVC_RSPDOR_PD_ENTRY	pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;

			if(NULL == (pLen = p2p_add_IEHdr(pBuf))) break;

			// Comment out because already appended by P2PConstructProvisionDiscoveryReq
			// P2P-cap, always present
			//bytesCopied = P2PMakeP2PCapability(pP2PInfo, Type_Action, osAttr.Octet + osAttr.Length);
			//osAttr.Length += bytesCopied;

			// Comment out because already appended by P2PConstructProvisionDiscoveryReq
			// Dev-info, always present
			//bytesCopied = P2PMakeP2PDeviceInfo(pP2PInfo, Type_Action, osAttr.Octet + osAttr.Length);
			//osAttr.Length += bytesCopied;

			// Group-id, contains NEW or GO
			if(TEST_FLAG(pRspdorPDEntry->fopdReqConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
			{
				FrameBuf_Add_u1(pBuf, P2P_ATTR_GROUP_ID);
				FrameBuf_Add_le_u2(pBuf, 6 + pPDEntry->selfGrpInfo.ssidLen);
				FrameBuf_Add_Data(pBuf, pP2PInfo->DeviceAddress, 6);
				FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.ssidBuf, pPDEntry->selfGrpInfo.ssidLen);
			}
			
			// Intended-intf-addr, contains NEW or GO
			if(TEST_FLAG(pRspdorPDEntry->fopdReqConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
			{
				FrameBuf_Add_u1(pBuf, P2P_ATTR_INTENDED_INTERFACE_ADDRESS);
				FrameBuf_Add_le_u2(pBuf, 6);
				FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.grpIntfAddr, 6);
			}

			// Status, always present
			FrameBuf_Add_u1(pBuf, P2P_ATTR_STATUS);
			FrameBuf_Add_le_u2(pBuf, 1);
			FrameBuf_Add_u1(pBuf, (u1Byte)pRspdorPDEntry->fopdReqStatus);

			// Op-channel, contains NEW or GO
			if(TEST_FLAG(pRspdorPDEntry->fopdReqConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
			{
				P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->OperatingChannel);	
			}

			// Channel-list, contains NEW
			if(TEST_FLAG(pRspdorPDEntry->fopdReqConnCap, P2PSVC_CONN_CAP_BMP_NEW))
			{
				P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pP2PInfo->ChannelEntryList);
			}
			
			// Session-info-data-info, NONE

			// Conn-cap, present if accepted
			if(P2P_STATUS_SUCCESS_ACCEPTED_BY_USER == pRspdorPDEntry->fopdReqStatus)
			{
				FrameBuf_Add_u1(pBuf, P2P_ATTR_CONN_CAP_INFO);
				FrameBuf_Add_le_u2(pBuf, 1);
				FrameBuf_Add_u1(pBuf, pRspdorPDEntry->fopdReqConnCap);
			}

			// Adv-id (adv-id, svc-mac), always present
			FrameBuf_Add_u1(pBuf, P2P_ATTR_ADV_ID_INFO);
			FrameBuf_Add_le_u2(pBuf, 4 + 6);
			FrameBuf_Add_le_u4(pBuf, pRspdorPDEntry->super.advId);
			FrameBuf_Add_Data(pBuf, pP2PInfo->DeviceAddress, 6);

			// Config-timeout, contains NEW or GO
			if(TEST_FLAG(pRspdorPDEntry->fopdReqConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
			{
				P2PAttr_Make_ConfigTimeout(pBuf, pP2PInfo->GOConfigurationTimeout, pP2PInfo->ClientConfigurationTimeout);
			}

			// Listen-channel, contains NEW or CLI
			if(TEST_FLAG(pRspdorPDEntry->fopdReqConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_CLI))
			{
				P2PAttr_Make_ListenChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->ListenChannel);
			}
			
			// Session-id-info, always present
			FrameBuf_Add_u1(pBuf, P2P_ATTR_SESSION_ID_INFO);
			FrameBuf_Add_le_u2(pBuf, 4 + 6);
			FrameBuf_Add_le_u4(pBuf, pRspdorPDEntry->super.sessionId);
			FrameBuf_Add_Data(pBuf, pRspdorPDEntry->super.peerDevAddr, 6);

			// Feature-cap, always present
			FrameBuf_Add_u1(pBuf, P2P_ATTR_FEATURE_CAP_INFO);
			FrameBuf_Add_le_u2(pBuf, 2);
			FrameBuf_Add_le_u2(pBuf, 0x0001);

			// Persistent-grp-info, present if persistent grp available and ASP intend using it
			// TODO:

			//4 Indicate P2P attrs sent
			P2PSvc_IndicateFrameSent(pP2PSvcInfo, 
				P2PSVC_OBJ_HDR_ID_INDIC_RSPDOR_FOPD_REQ_SENT, 
				pRspdorPDEntry->super.peerDevAddr, 
				(u2Byte)(FrameBuf_Tail(pBuf) - 1 - pLen), 
				pLen + 1);

			p2p_update_IeHdrLen(pBuf, pLen);
		}while(FALSE);

		P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

		return rtStatus;
}

RT_STATUS
P2PSvc_MakeInitorFOPDRspIE(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  POCTET_STRING 				posP2PAttrs,
	OUT FRAME_BUF					*pBuf
	) 
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO						pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	PP2PSVC_INITOR_PD_ENTRY			pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;
	pu1Byte							pLen = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		OCTET_STRING				osConnCap = {NULL, 0};
		PRT_OBJECT_HEADER 			pObj = NULL;
		pu1Byte						peerDevAddr = NULL;
		u4Byte						advId = 0;
		PP2P_WPS_ATTRIBUTES 		pWps = &pP2PInfo->WpsAttributes;
		
		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_CONN_CAP_INFO, 1, &osConnCap))
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
		
		if(NULL == (pLen = p2p_add_IEHdr(pBuf))) break;
		
		// P2P-cap, always present
		if(p2p_ActingAs_Go(pP2PInfo))
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));
		P2PAttr_Make_Capability(pBuf, 
			pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
			pP2PInfo->GroupCapability);

		// Dev-info, always present
		P2PAttr_Make_DevInfo(pBuf, 
			pP2PInfo->DeviceAddress, 
			pWps->ConfigMethod, 
			&pWps->PrimaryDeviceType, 
			pWps->SecondaryDeviceTypeLength, 
			pWps->SecondaryDeviceTypeList,
			pWps->DeviceNameLength,
			pWps->DeviceName);

		// Group-id, connCap IS GO
		if(P2PSVC_CONN_CAP_BMP_GO == pInitorPDEntry->fopdRspConnCap)
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_GROUP_ID);
			FrameBuf_Add_le_u2(pBuf, 6 + pPDEntry->selfGrpInfo.ssidLen);
			FrameBuf_Add_Data(pBuf, pP2PInfo->DeviceAddress, 6);
			FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.ssidBuf, pPDEntry->selfGrpInfo.ssidLen);
		}
		
		// Intended-intf-addr, connCap IS GO
		if(P2PSVC_CONN_CAP_BMP_GO == pInitorPDEntry->fopdRspConnCap)
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_INTENDED_INTERFACE_ADDRESS);
			FrameBuf_Add_le_u2(pBuf, 6);
			FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.grpIntfAddr, 6);
		}

		// Status, always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_STATUS);
		FrameBuf_Add_le_u2(pBuf, 1);
		FrameBuf_Add_u1(pBuf, (u1Byte)pInitorPDEntry->fopdRspStatus);

		// Op-channel, connCap IS GO
		if(P2PSVC_CONN_CAP_BMP_GO == pInitorPDEntry->fopdRspConnCap)
		{
			P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->OperatingChannel);	
		}

		// Channel-list, IS CLI or GO
		if(P2PSVC_CONN_CAP_BMP_GO == pInitorPDEntry->fopdRspConnCap
			|| P2PSVC_CONN_CAP_BMP_CLI == pInitorPDEntry->fopdRspConnCap 
			)
		{
			P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pP2PInfo->ChannelEntryList);
		}
		
		// Session-info-data-info, NONE

		// Conn-cap, always present?
		if(P2P_STATUS_SUCCESS == pInitorPDEntry->fopdRspStatus)
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_CONN_CAP_INFO);
			FrameBuf_Add_le_u2(pBuf, 1);
			FrameBuf_Add_u1(pBuf, pInitorPDEntry->fopdRspConnCap);
		}
		
		// Adv-id (adv-id, svc-mac), always present
		pObj = P2PSvc_GetParam(&pInitorPDEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, 0);
		peerDevAddr = pObj->Value;
		pObj = P2PSvc_GetParam(&pInitorPDEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
		advId = *(pu4Byte)pObj->Value;

		FrameBuf_Add_u1(pBuf, P2P_ATTR_ADV_ID_INFO);
		FrameBuf_Add_le_u2(pBuf, 4 + 6);
		FrameBuf_Add_le_u4(pBuf, advId);
		FrameBuf_Add_Data(pBuf, pInitorPDEntry->super.peerDevAddr, 6);

		// Config-timeout, IS GO or CLI
		// Always add config-time out since WFA is checking this even when PD is failed
		//if(P2PSVC_CONN_CAP_BMP_GO == pInitorPDEntry->fopdRspConnCap
		//	|| P2PSVC_CONN_CAP_BMP_CLI == pInitorPDEntry->fopdRspConnCap 
		//	)
		{
			P2PAttr_Make_ConfigTimeout(pBuf, pP2PInfo->GOConfigurationTimeout, pP2PInfo->ClientConfigurationTimeout);
		}

		// Listen-channel, NONE
		
		// Session-id-info, always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_SESSION_ID_INFO);
		FrameBuf_Add_le_u2(pBuf, 4 + 6);
		FrameBuf_Add_le_u4(pBuf, pInitorPDEntry->super.sessionId);
		FrameBuf_Add_Data(pBuf, pP2PInfo->DeviceAddress, 6);

		// Feature-cap, always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_FEATURE_CAP_INFO);
		FrameBuf_Add_le_u2(pBuf, 2);
		FrameBuf_Add_le_u2(pBuf, 0x0001);

		// Persistent-grp-info, NONE

		//4 Indicate P2P attrs sent
		P2PSvc_IndicateFrameSent(pP2PSvcInfo, 
			P2PSVC_OBJ_HDR_ID_INDIC_INITOR_FOPD_RSP_SENT, 
			peerDevAddr, 
			(u2Byte)(FrameBuf_Tail(pBuf) - 1 - pLen), 
			pLen + 1);

		p2p_update_IeHdrLen(pBuf, pLen);
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;

}

RT_STATUS
P2PSvc_MakeRspdorPDRspIE(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  POCTET_STRING 				posP2PAttrs,
	OUT FRAME_BUF					*pBuf
	) 
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO						pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	PP2PSVC_RSPDOR_PD_ENTRY			pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;
	pu1Byte							pLen = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PP2P_WPS_ATTRIBUTES 		pWps = &pP2PInfo->WpsAttributes;
		
		if(NULL == (pLen = p2p_add_IEHdr(pBuf))) break;

		// P2P-cap, always present
		if(p2p_ActingAs_Go(pP2PInfo))
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));
		P2PAttr_Make_Capability(pBuf, 
			pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
			pP2PInfo->GroupCapability);

		// P2P-dev-info, always present
		P2PAttr_Make_DevInfo(pBuf, 
			pP2PInfo->DeviceAddress, 
			pWps->ConfigMethod, 
			&pWps->PrimaryDeviceType, 
			pWps->SecondaryDeviceTypeLength, 
			pWps->SecondaryDeviceTypeList,
			pWps->DeviceNameLength,
			pWps->DeviceName);

		// Group-id, IS GO
		if(P2PSVC_CONN_CAP_BMP_GO == pRspdorPDEntry->pdRspConnCap)
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_GROUP_ID);
			FrameBuf_Add_le_u2(pBuf, 6 + pPDEntry->selfGrpInfo.ssidLen);
			FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.grpDevAddr, 6);
			FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.ssidBuf, pPDEntry->selfGrpInfo.ssidLen);
		}
		
		// Intended-intf-addr, contains NEW or GO
		if(TEST_FLAG(pRspdorPDEntry->pdRspConnCap, P2PSVC_CONN_CAP_BMP_GO))
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_INTENDED_INTERFACE_ADDRESS);
			FrameBuf_Add_le_u2(pBuf, 6);
			FrameBuf_Add_Data(pBuf, pPDEntry->selfGrpInfo.grpIntfAddr, 6);
		}

		// Status, always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_STATUS);
		FrameBuf_Add_le_u2(pBuf, 1);
		FrameBuf_Add_u1(pBuf, pRspdorPDEntry->pdRspStatus);

		// Operating-channel, IS GO
		if(P2PSVC_CONN_CAP_BMP_GO == pRspdorPDEntry->pdRspConnCap)
		{
			P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->OperatingChannel);
		}

		// Channel-list, IS CLI or GO
		if(P2PSVC_CONN_CAP_BMP_GO == pRspdorPDEntry->pdRspConnCap 
			|| P2PSVC_CONN_CAP_BMP_CLI == pRspdorPDEntry->pdRspConnCap 
			)
		{
			P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pP2PInfo->ChannelEntryList);
		}

		// session-info-data-info, NONE

		// conn-cap, always present, present if SUCCESS
		if(P2P_STATUS_SUCCESS == pRspdorPDEntry->pdRspStatus)
		{
			FrameBuf_Add_u1(pBuf, P2P_ATTR_CONN_CAP_INFO);
			FrameBuf_Add_le_u2(pBuf, 1);
			FrameBuf_Add_u1(pBuf, pRspdorPDEntry->pdRspConnCap);
		}

		// adv-id (adv-id, svc-mac), always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_ADV_ID_INFO);
		FrameBuf_Add_le_u2(pBuf, 4 + 6);
		FrameBuf_Add_le_u4(pBuf, pRspdorPDEntry->super.advId);
		FrameBuf_Add_Data(pBuf, pP2PInfo->DeviceAddress, 6);

		// Config-timeout, IS GO or CLI
		// Always add config-timeout since WFA is checking this even status is failed
		//if(P2PSVC_CONN_CAP_BMP_GO == pRspdorPDEntry->pdRspConnCap 
		//	|| P2PSVC_CONN_CAP_BMP_CLI == pRspdorPDEntry->pdRspConnCap 
		//	)
		{	
			P2PAttr_Make_ConfigTimeout(pBuf, pP2PInfo->GOConfigurationTimeout, pP2PInfo->ClientConfigurationTimeout);
		}

		// Listen-channel, NONE
		
		// Session-id-info, always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_SESSION_ID_INFO);
		FrameBuf_Add_le_u2(pBuf, 4 + 6);
		FrameBuf_Add_le_u4(pBuf, pRspdorPDEntry->super.sessionId);
		FrameBuf_Add_Data(pBuf, pRspdorPDEntry->super.peerDevAddr, 6);

		// Feature-cap, always present
		FrameBuf_Add_u1(pBuf, P2P_ATTR_FEATURE_CAP_INFO);
		FrameBuf_Add_le_u2(pBuf, 2);
		FrameBuf_Add_le_u2(pBuf, 0x0001);

		// Persistent-grp-info, present if persistent grp known and ASP intent using it
		// TODO:

		//4 Indicate P2P attrs sent
		P2PSvc_IndicateFrameSent(pP2PSvcInfo, 
			P2PSVC_OBJ_HDR_ID_INDIC_RSPDOR_PD_RSP_SENT, 
			pRspdorPDEntry->super.peerDevAddr, 
			(u2Byte)(FrameBuf_Tail(pBuf) - 1 - pLen), 
			pLen + 1);
		
		p2p_update_IeHdrLen(pBuf, pLen);
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;

}

RT_STATUS
P2PSvc_MakeSvcInfoDesc(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_REQ_INFO_ENTRY		pAdvInfoEntry,
	IN  u1Byte						svcInfoReqLen,
	IN  pu1Byte						pSvcInfoReqBuf,
	OUT FRAME_BUF					*pBuf
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO						pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	PP2PSVC_OBJ_LIST				pAdvSvcObjList = &pAdvInfoEntry->objList;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PRT_OBJECT_HEADER 			pAdvIdObj = NULL;
		PRT_OBJECT_HEADER			pNetworkConfigObj = NULL;
		PRT_OBJECT_HEADER 			pSvcNameObj = NULL;
		PRT_OBJECT_HEADER			pSvcStatusObj = NULL;
		PRT_OBJECT_HEADER			pSvcInfoObj = NULL;

		u2Byte						svcConfigMethod = pP2PInfo->WpsAttributes.ConfigMethod;

		// Get info from the advertised svc
		if(NULL == (pAdvIdObj = P2PSvc_GetParam(pAdvSvcObjList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0)))
		{
			break;
		}

		if(NULL == (pNetworkConfigObj = P2PSvc_GetParam(pAdvSvcObjList, P2PSVC_OBJ_HDR_ID_DATA_NETWORK_CONFIG, 0)))
		{
			break;
		}

		if(NULL == (pSvcNameObj = P2PSvc_GetParam(pAdvSvcObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 0)))
		{
			break;
		}

		if(NULL == (pSvcStatusObj = P2PSvc_GetParam(pAdvSvcObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS, 0)))
		{
			break;
		}

		if(NULL == (pSvcInfoObj = P2PSvc_GetParam(pAdvSvcObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_INFO, 0)))
		{
			break;
		}

		// Construct svc info desc (ref addendum v0.43 Table G3)
		// [adv-id(4), config-method(2), svc-name-len(1), svc-name(n), svc-status(1), svc-info-len(2), svc-info(n)]

		// adv-id
		FrameBuf_Add_le_u4(pBuf, *(pu4Byte)pAdvIdObj->Value);

		// config-method
		// TODO: (v0.43) set to the config method set from upper layer
		//P2PSVC_SET_ATTR_2Byte(&rtStatus, posSvcInfoDesc, maxBufLen, 1);
		if(2 == *(PUCHAR)pNetworkConfigObj->Value)
		{// WSC PIN method only
			CLEAR_FLAG(svcConfigMethod, P2P_WPS_CONFIG_METHODS_SVC_DEFAULT_PIN);
		}
		FrameBuf_Add_be_u2(pBuf, svcConfigMethod);
		
		// svc-name-len
		FrameBuf_Add_u1(pBuf, (u1Byte)pSvcNameObj->Length);

		// svc-name
		FrameBuf_Add_Data(pBuf, pSvcNameObj->Value, (u2Byte)pSvcNameObj->Length);

		// svc-status
		FrameBuf_Add_u1(pBuf, *(pu1Byte)pSvcStatusObj->Value);

		// svc-info
		if(P2PSvc_MatchSubstring(pSvcInfoObj->Value, pSvcInfoObj->Length, pSvcInfoReqBuf, svcInfoReqLen))
		{
			// svc-info-len
			FrameBuf_Add_le_u2(pBuf, (u2Byte)pSvcInfoObj->Length);
			
			// svc-info
			FrameBuf_Add_Data(pBuf, pSvcInfoObj->Value, (u2Byte)pSvcInfoObj->Length);
		}
		else
		{
			// svc-info-len
			FrameBuf_Add_le_u2(pBuf, 0);
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)
