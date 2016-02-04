#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Build_Action.tmh"
#endif

#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

static
VOID
p2p_add_P2PActionHdr(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					subtype,
	IN  u1Byte					dialogToken
	)
{
	FrameBuf_Add_u1(pBuf, WLAN_ACTION_VENDOR_SPECIFIC);
	FrameBuf_Add_be_u4(pBuf, P2P_IE_VENDOR_TYPE);
	FrameBuf_Add_u1(pBuf, subtype);
	FrameBuf_Add_u1(pBuf, dialogToken);

	return;
}

static
VOID
p2p_build_PresenceReqIe(
	IN  FRAME_BUF				*pBuf,
	IN  const P2P_POWERSAVE_SET	*pPs
	)
{
	pu1Byte						pLen = NULL;
	
	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	P2PAttr_Make_NoaFromPsSet(pBuf, pPs);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_PresenceRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					status,
	IN  const P2P_INFO			*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;
	
	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;
		
	P2PAttr_Make_Status(pBuf, status);
	P2PAttr_Make_Noa(pBuf, 
		pP2PInfo->NoAIEIndex, 
		pP2PInfo->bOppPS, 
		pP2PInfo->CTWindow, 
		P2P_MAX_NUM_NOA_DESC, 
		pP2PInfo->NoADescriptors);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_GoDiscoverabilityReq(
	IN  FRAME_BUF				*pBuf,
	IN  const P2P_INFO			*pP2PInfo
	)
{
	// There is no Element field in a GO Discoverability Request frame.
	return;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

VOID
p2p_Construct_PresenceReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const P2P_POWERSAVE_SET	*pP2pPs,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken,
	IN  FRAME_BUF 				*pBuf
	)
{
	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, da);

	// Action Header
	p2p_add_P2PActionHdr(pBuf, P2P_PRESENCE_REQ, dialogToken);

	// P2P IE
	p2p_build_PresenceReqIe(pBuf, pP2pPs);

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);
	
	return;
}

VOID
p2p_Construct_PresenceRsp(
	IN  P2P_INFO				*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte					dialogToken,
	IN  u1Byte					status,
	IN  FRAME_BUF 				*pBuf
	)
{	
	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, pP2PInfo->DeviceAddress);

	// Action Header
	p2p_add_P2PActionHdr(pBuf, P2P_PRESENCE_RSP, dialogToken);

	// P2P IE
	p2p_build_PresenceRspIe(pBuf, status, pP2PInfo);

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);

	return;
}

VOID
p2p_Copnstruct_GoDiscoverabilityReq(
	IN  P2P_INFO				*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte					dialogToken,
	IN  FRAME_BUF 				*pBuf
	)
{	
	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, pP2PInfo->InterfaceAddress);

	// Action Header
	p2p_add_P2PActionHdr(pBuf, P2P_GO_DISC_REQ, dialogToken);

	// P2P IE
	p2p_build_GoDiscoverabilityReq(pBuf, pP2PInfo);

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);

	return;
}

VOID 
p2p_Construct_SDReq(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	)
{
	u1Byte 						*pQueryReqLen = NULL;
	u1Byte 						*pAnqpQueryReqLen = NULL;
	u1Byte 						itSvcTlv = 0;

	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, da);

	// Action Header
	p2p_add_PublicActionHdr(pBuf, WLAN_PA_GAS_INITIAL_REQ, pP2PInfo->SDContext.DialogToken);

	// Advertisement Protocol Info IE
	FrameBuf_Add_u1(pBuf, 108);		// Element ID
	FrameBuf_Add_u1(pBuf, 2);		// Legnth
	FrameBuf_Add_u1(pBuf, 0);		// Query Response Length Limit and PAME-BI
	FrameBuf_Add_u1(pBuf, 0);		// Advertisment Protocol ID
	
	//Query Request Length
	pQueryReqLen = FrameBuf_Add(pBuf, 2);

	// ANQP Query Request 
	FrameBuf_Add_u1(pBuf, 0xDD); 					// Info ID, first
	FrameBuf_Add_u1(pBuf, 0xDD); 					// Info ID, second
	pAnqpQueryReqLen = FrameBuf_Add(pBuf, 2);		// Lenghth
	FrameBuf_Add_be_u4(pBuf, P2P_IE_VENDOR_TYPE); 	// OUI (OI) and subtype
	FrameBuf_Add_le_u2(pBuf, 0);					// Service update indicator, in req we always fill 0

	// Service TLVs
	for(itSvcTlv = 0; itSvcTlv < pP2PInfo->SDContext.UserSDReq.ServiceReqTLVSize; itSvcTlv++)
	{
		const P2P_SERVICE_REQ_TLV 	*tlv = &(pP2PInfo->SDContext.UserSDReq.ServiceReqTLVList[itSvcTlv]);
		u1Byte						*pContentLen = NULL;

		pContentLen = FrameBuf_Add(pBuf, 2);					// Length
		FrameBuf_Add_u1(pBuf, tlv->ServiceDesc.ServiceType);	// Service Protocol Type
		FrameBuf_Add_u1(pBuf, tlv->TransactionID);				// Service Transaction ID
		FrameBuf_Add_Data(pBuf, 								// Query Data
			tlv->ServiceDesc.Buffer, 
			tlv->ServiceDesc.BufferLength); 

		WriteEF2Byte(pContentLen, 								// Write back length
			FrameBuf_Tail(pBuf) - (pContentLen + 2));
	}

	WriteEF2Byte(pQueryReqLen, FrameBuf_Tail(pBuf) - (pQueryReqLen + 2));			// Write back query length
	WriteEF2Byte(pAnqpQueryReqLen, FrameBuf_Tail(pBuf) - (pAnqpQueryReqLen + 2));	// Write back ANQP query length

	FunctionOut(COMP_P2P);

	return;
}

VOID 
p2p_Construct_SDRsp(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  P2P_SD_STATUS_CODE		status,
	IN  BOOLEAN					bFrag
	)
{
	u1Byte 						*pQueryRspLen = NULL;

	FunctionIn(COMP_P2P);
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bFragment: %u\n", bFrag));
	
	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, pP2PInfo->DeviceAddress);

	// Action Header
	p2p_add_PublicActionHdr(pBuf, WLAN_PA_GAS_INITIAL_RESP, dialogToken);

	// Status code (11u): 2 octets
	FrameBuf_Add_le_u2(pBuf, 0);

	// GAS Comeback Delay: 0: no frag, 1: frag
	FrameBuf_Add_le_u2(pBuf, bFrag);

	// Advertisement Protocol Info IE
	FrameBuf_Add_u1(pBuf, 108);		// Element ID
	FrameBuf_Add_u1(pBuf, 2);		// Legnth
	FrameBuf_Add_u1(pBuf, 0x7F);	// Query Response Length Limit and PAME-BI. 
									// The Response Length set to 1111111 PAME-BI is 0
	FrameBuf_Add_u1(pBuf, 0);		// Advertisment Protocol ID

	// Query Request Length
	pQueryRspLen = FrameBuf_Add(pBuf, 2);

	// ANQP Query Response
	if(!bFrag)
	{// append the entire ANQP Query Rsp Field
		FrameBuf_Add_Data(pBuf, 
			pP2PInfo->SDContext.ANQPQueryRspFieldToSendBuf, 
			pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize);

		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Adding query rsp:\n",
			pP2PInfo->SDContext.ANQPQueryRspFieldToSendBuf, 
			pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize);
	}

	WriteEF2Byte(pQueryRspLen, FrameBuf_Tail(pBuf) - (pQueryRspLen + 2)); // Write back length

	FunctionOut(COMP_P2P);

	return;
}

VOID 
p2p_Construct_SDComebackReq(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken
	)
{	
	FunctionIn(COMP_P2P);
	
	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, da);
	
	// Action Header
	p2p_add_PublicActionHdr(pBuf, WLAN_PA_GAS_COMEBACK_REQ, dialogToken);

	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_SDComebackRsp(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  u2Byte					bytesToCopy,
	IN  BOOLEAN					bMoreData
	)
{	
	//
	// Note: if comeback is used, we send only 1 Service Rsp TLV in the ComebackRsp
	//

	u1Byte						fragId = 0;
	u1Byte						*pQueryRspLen = NULL;

	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, pP2PInfo->DeviceAddress);

	// Action Header
	p2p_add_PublicActionHdr(pBuf, WLAN_PA_GAS_COMEBACK_RESP, dialogToken);

	// Status code (11u): 2 octets
	FrameBuf_Add_le_u2(pBuf, 0);			

	// GAS Query Response Fragment ID
	fragId = pP2PInfo->SDContext.FragmentID;
	if(bMoreData) 
		SET_FLAG(fragId, BIT7);
	FrameBuf_Add_u1(pBuf, fragId);

	// GAS Comeback Delay: 0: no frag, 1: frag
	FrameBuf_Add_le_u2(pBuf, 0);

	// Advertisement Protocol Info IE
	FrameBuf_Add_u1(pBuf, 108);		// Element ID
	FrameBuf_Add_u1(pBuf, 2);		// Legnth
	FrameBuf_Add_u1(pBuf, 0x7F);	// Query Response Length Limit and PAME-BI. 
									// The Response Length set to 1111111 PAME-BI is 0
	FrameBuf_Add_u1(pBuf, 0);		// Advertisment Protocol ID

	// Query Request Length
	pQueryRspLen = FrameBuf_Add(pBuf, 2);

	// Payload
	FrameBuf_Add_Data(pBuf, 
		pP2PInfo->SDContext.ANQPQueryRspFieldToSendBuf + pP2PInfo->SDContext.ANQPQueryRspFieldToSendOffset, 
		bytesToCopy);

	WriteEF2Byte(pQueryRspLen, FrameBuf_Tail(pBuf) - (pQueryRspLen + 2)); // Write back length

	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_AnqpQueryRspField(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  P2P_SD_RSP_CONTEXT 		*pRspCtx,
	OUT P2P_SD_CONTEXT			*pSdCtx
	)
{
	//
	// We construct an un-fragmented ANQP Query Rsp Field according to the Service Req TLVs
	//
	
	FRAME_BUF					fbuf;
	
	const P2P_SERVICE_RSP_TLV 	*pSvcRspTLVs = pRspCtx->ServiceRspTLVList;
	u1Byte						*pAnqpQueryRspLen = NULL;

	FrameBuf_Init(sizeof(pSdCtx->ANQPQueryRspFieldToSendBuf), 
		0, 
		pSdCtx->ANQPQueryRspFieldToSendBuf, 
		&fbuf);

	FrameBuf_Add_u1(&fbuf, 0xDD);
	FrameBuf_Add_u1(&fbuf, 0xDD);
	pAnqpQueryRspLen = FrameBuf_Add(&fbuf, 2);

	FrameBuf_Add_be_u4(&fbuf, P2P_IE_VENDOR_TYPE); 					// OUI (OI) and subtype
	FrameBuf_Add_le_u2(&fbuf, pRspCtx->ServiceUpdateIndicator); 	// Service update indicator

	// Service TLVs, it could be multi TLV for multi service type
	if(pRspCtx->SDStatus == P2P_SD_STATUS_SUCCESS)
	{
		u1Byte					itSvcTlv = 0;
		
		for(itSvcTlv = 0; itSvcTlv < pRspCtx->ServiceRspTLVSize; itSvcTlv++)
		{
			u1Byte					*pLen = NULL;
			
			pLen = FrameBuf_Add(&fbuf, 2);
			FrameBuf_Add_u1(&fbuf, pSvcRspTLVs[itSvcTlv].ServiceDesc.ServiceType);	// Service Protocol Type
			FrameBuf_Add_u1(&fbuf, pSvcRspTLVs[itSvcTlv].TransactionID);			// Service Transation ID
			FrameBuf_Add_u1(&fbuf, pSvcRspTLVs[itSvcTlv].Status);					// Status
			FrameBuf_Add_Data(&fbuf, 												// Response Data
				pSvcRspTLVs[itSvcTlv].ServiceDesc.Buffer, 
				pSvcRspTLVs[itSvcTlv].ServiceDesc.BufferLength);
			WriteEF2Byte(pLen, FrameBuf_Tail(&fbuf) - (pLen + 2));					// Write back length
		}
	}
	else
	{
		u1Byte					*pLen = NULL;

		pLen = FrameBuf_Add(&fbuf, 2);
		FrameBuf_Add_u1(&fbuf, P2P_SD_PROTOCOL_ALL_TYPE);		// Service Protocol Type
		FrameBuf_Add_u1(&fbuf, 0);								// Service Transation ID
		FrameBuf_Add_u1(&fbuf, pRspCtx->SDStatus);				// Status

		WriteEF2Byte(pLen, FrameBuf_Tail(&fbuf) - (pLen + 2)); 	// Write back length
	}

	pSdCtx->ANQPQueryRspFieldToSendSize = FrameBuf_Length(&fbuf);

	return;
}

#endif
