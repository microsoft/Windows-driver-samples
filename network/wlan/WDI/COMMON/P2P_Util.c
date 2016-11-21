#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Util.tmh"
#endif

#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

VOID
p2p_MoveMemory(
	IN  VOID				*dest,
	IN  const VOID			*src,
	IN  u4Byte				len
	)
{
	u4Byte 					it = 0; 

	for(it = 0; it < len; it++)
		*(((u1Byte *)dest) + it) = *(((u1Byte *)src) + it);
}

int
p2p_go_det(
	IN  u1Byte					selfVal, 
	IN  u1Byte					peerVal
	)
{
	u1Byte						selfIntent = selfVal >> 1;
	u1Byte						peerIntent = peerVal >> 1;

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("self intent: %u, tie: %u, peer intent: %u, tie: %u\n",
		selfVal >> 1, selfVal & 0x01, peerVal >> 1, peerVal & 0x01));
	
	if(selfIntent == peerIntent)
	{
		if (P2P_MAX_GO_INTENT == selfIntent)
			return -1; /* both devices want to become GO */

		/* Use tie breaker bit to determine GO */
		return (peerVal & 0x01) ? 0 : 1;
	}

	return selfIntent > peerIntent;
}


BOOLEAN
p2p_ActingAs_Go(
	IN  P2P_INFO 				*pP2PInfo
	) 
{
	BOOLEAN						bGo = FALSE;
	ADAPTER 					*pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) 
									? pP2PInfo->pAdapter 
								: GetFirstGOPort(pP2PInfo->pAdapter);

	bGo = (P2P_GO == pP2PInfo->Role && ACTING_AS_AP(pExtAdapter)); // copied from P2PMakeP2PCapability

	if(bGo)
	{
		SET_FLAG(pP2PInfo->GroupCapability, P2P_GROUP_CAP_GROUP_OWNER);
	}
	else
	{
		CLEAR_FLAG(pP2PInfo->GroupCapability, P2P_GROUP_CAP_GROUP_OWNER);
	}

	return bGo;
}

BOOLEAN
p2p_Doing_Provisioning(
	IN  const P2P_INFO 			*pP2PInfo
	)
{
	PMGNT_INFO	pDefaultMgntInfo = &(GetDefaultAdapter(pP2PInfo->pAdapter)->MgntInfo);	

	if(0 < GET_SIMPLE_CONFIG_IE_LEN(pDefaultMgntInfo) && GET_SIMPLE_CONFIG_ENABLED(pDefaultMgntInfo))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN
p2p_Check_GroupLimitReached(
	IN  P2P_INFO 				*pP2PInfo
	)
{
	if(p2p_ActingAs_Go(pP2PInfo))
	{
		if(P2P_MAX_P2P_CLIENT <= P2PClientInfoGetCount(pP2PInfo))
		{// no further P2P connections are supported
			SET_FLAG(pP2PInfo->GroupCapability, P2P_GROUP_CAP_GROUP_LIMIT);
			return TRUE;
		}
	}

	return FALSE;
}

P2P_DEV_TYPE
p2p_MessageSrcType(
	IN  const P2P_MESSAGE		*msg,
	IN  P2P_FRAME_TYPE			frameType
	)
{
	if(frameType < P2P_FID_ACTION_START
		&& !FrameBuf_Length(&msg->p2pAttributes)
		)
	{
		return P2P_DEV_TYPE_LEGACY;
	}
			
	if(msg->_dialogToken)
	{// action frame
		if(msg->_capability)
		{
			if(TEST_FLAG(msg->grpCap, P2P_GROUP_CAP_GROUP_OWNER))
			{
				if(P2P_FID_GO_NEG_REQ == frameType
					|| P2P_FID_GO_NEG_RSP == frameType
					|| P2P_FID_GO_NEG_CONF == frameType
					)
				{
					return P2P_DEV_TYPE_DEV; // Actiontec ScreenBeam sends GO Neg Rsp with GO bit set
				}
				return P2P_DEV_TYPE_GO;
			}
			else
				return P2P_DEV_TYPE_DEV;
		}

		return P2P_DEV_TYPE_DEV;
	}
	else
	{
		if(!FrameBuf_Length(&msg->p2pAttributes))
			return P2P_DEV_TYPE_LEGACY;

		if(msg->_capability)
		{
			if(TEST_FLAG(msg->grpCap, P2P_GROUP_CAP_GROUP_OWNER))
				return P2P_DEV_TYPE_GO;
			else
				return P2P_DEV_TYPE_DEV;
		}
	}

	return P2P_DEV_TYPE_LEGACY;
}

BOOLEAN
p2p_FindClientInfoByDevAddr(
	IN  const u1Byte			*devAddr,
	IN  const P2P_MESSAGE		*msg,
	OUT const u1Byte			**ppCliInfo
	)
{
	BOOLEAN						bFound = FALSE;
	
	if(msg->_grpInfo && TEST_FLAG(msg->grpCap, P2P_GROUP_CAP_GROUP_OWNER))
	{
		const u1Byte 			*pos = msg->cliInfoDescList;
		const u1Byte 			*end = msg->cliInfoDescList + msg->cliInfoDescListLen;

		while(pos < end)
		{
			u1Byte				len = 0;
			const u1Byte		*infostart = NULL;
			const u1Byte 		*infoend = NULL;

			infostart = pos;

			len = pos[0];
			pos += 1;
			
			infoend = pos + len;
			if(end < infoend)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("client info length field overflow: %u\n", len));
				break;
			}

			if(len < 6 + 6 + 1 + 2 + 8 + 1)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid client info length field: %u\n", len));
				break;
			}

			if(eqMacAddr(pos, devAddr))
			{
				bFound = TRUE;
				if(ppCliInfo)
					*ppCliInfo = infostart;
				break;
			}
			else
			{
				pos = infoend;
			}
		}
	}	

	return bFound;
}

#endif