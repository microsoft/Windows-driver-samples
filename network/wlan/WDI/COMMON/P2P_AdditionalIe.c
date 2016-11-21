#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_AdditionalIe.tmh"
#endif

#include "P2P_AdditionalIe.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

VOID
P2P_AddIe_Init(
	IN  PP2P_ADD_IES			addIes
	)
{
	PlatformZeroMemory(addIes, sizeof(*addIes));

	return;
}

VOID
P2P_AddIe_Free(
	IN  PP2P_ADD_IES 			addIes
	)
{
	u4Byte						itAddIe = 0;

	for(itAddIe = 0; itAddIe < P2P_ADD_IE_MAX; itAddIe++)
	{
		if(addIes->addIe[itAddIe])
		{
			FrameBuf_Free(addIes->addIe[itAddIe]);
			addIes->addIe[itAddIe] = NULL;
		}
	}
}

BOOLEAN
P2P_AddIe_Set(
	IN  PP2P_ADD_IES			addIes,
	IN  P2P_ADD_IE_ID			id,
	IN  u4Byte					bufLen,
	IN  u1Byte					*pBuf
	)
{
	FRAME_BUF					srcBuf;
	FRAME_BUF					*pClonedSrcBuf = NULL;
	FRAME_BUF					**ppDestBuf = NULL;

	if(P2P_ADD_IE_MAX < id)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: invalid id: %u!\n", __FUNCTION__, id));
		return FALSE;
	}

	FrameBuf_Init((u2Byte)bufLen, (u2Byte)bufLen, pBuf, &srcBuf);


	ppDestBuf = &(addIes->addIe[id]);

	if(NULL == (pClonedSrcBuf = FrameBuf_Clone(&srcBuf)))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Frame buffer clone Failure!\n", __FUNCTION__));
		return FALSE;
	}

	if(*ppDestBuf) FrameBuf_Free(*ppDestBuf);

	*ppDestBuf = pClonedSrcBuf;
	
	return TRUE;
}

const FRAME_BUF *
P2P_AddIe_Get(
	IN  const PP2P_ADD_IES		addIes,
	IN  P2P_ADD_IE_ID			id
	)
{
	FRAME_BUF					*pBuf = NULL;

	if(P2P_ADD_IE_MAX < id)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: invalid id: %u!\n", __FUNCTION__, id));
		return NULL;
	}

	if (addIes->addIe != NULL)
	{
		// Prefast warning C6385: Reading invalid data from 'addIes->addIe':  the readable size is '44' bytes, but 'id' bytes may be read.
		// false positive, disable it here.
#pragma warning( disable: 6385 )
		pBuf = addIes->addIe[id];
	}

	return pBuf;
}

u2Byte
P2P_AddIe_Append(
	IN  const PP2P_ADD_IES		addIes,
	IN  P2P_ADD_IE_ID			id,
	IN  FRAME_BUF				*pBuf
	)
{
	const FRAME_BUF				*pAddIeBuf = NULL;
	u2Byte						bytesWritten = 0;

	pAddIeBuf = P2P_AddIe_Get(addIes, id);

	if(pAddIeBuf)
	{
		if(FrameBuf_Append(pBuf, pAddIeBuf))
		{
			bytesWritten = FrameBuf_Length(pAddIeBuf);
		}
		FrameBuf_Dump(pAddIeBuf, 0, DBG_LOUD, __FUNCTION__);
	}

	return bytesWritten;
}

#endif
