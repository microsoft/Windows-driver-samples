#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_SendMgnt.tmh"
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
p2p_Send_ProbeRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  RT_RFD					*rfdProbeReq,
	IN  OCTET_STRING			*posProbeReq,
	IN  OCTET_STRING			*posAttrs
	)
{
	PRT_TCB						pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pP2PInfo->pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;
		
		FrameBuf_Init(pP2PInfo->pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		
		if(p2p_Construct_ProbeRsp(&fbuf, pP2PInfo, da, rfdProbeReq, posProbeReq, posAttrs))
		{
			pTcb->PacketLength = FrameBuf_Length(&fbuf);
			MgntSendPacket(pP2PInfo->pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
		}
		else
		{
			pTcb->BufferType = RT_TCB_BUFFER_TYPE_LOCAL;
			pTcb->BufferList[0] = pBuf->Buffer;
			pTcb->Reserved = pBuf;
			ReturnTCB(pP2PInfo->pAdapter, pTcb, RT_STATUS_SUCCESS);
		}
	}
	
	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);
}

#endif
