#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Build.tmh"
#endif

#include "P2P_Internal.h"

#define P2P_BUILD_P2P_IE_HDR_LEN 		(1 + 1 + 4)
#define P2P_BUILD_MAX_ATTR_LEN_PER_IE 	(240)

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
static
VOID
p2p_write_IeLength(
	IN  FRAME_BUF				*pBuf,
	IN  pu1Byte					pLen
	)
{
	WriteEF1Byte(pLen, (u1Byte)((FrameBuf_Tail(pBuf) - 1) - pLen));
	return;
}

static
VOID
p2p_build_AttrIe(
	IN  FRAME_BUF				*pBuf,
	IN  const FRAME_BUF			*pAttr
	)
{
	const u1Byte				*pos = FrameBuf_Head(pAttr);
	const u1Byte				*end = pos + FrameBuf_Length(pAttr);

	while(pos < end)
	{
		u2Byte 					fragLen = (u2Byte)(end - pos);
		pu1Byte		pLen = NULL;

		if(P2P_BUILD_MAX_ATTR_LEN_PER_IE < fragLen)
			fragLen = P2P_BUILD_MAX_ATTR_LEN_PER_IE;

		if(NULL == (pLen = p2p_add_IEHdr(pBuf))) break;
	
		FrameBuf_Add_Data(pBuf, pos, fragLen);
	
		p2p_write_IeLength(pBuf, pLen);
	}

	return;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

u1Byte *
p2p_add_IEHdr(
	IN  FRAME_BUF				*pBuf
	)
{
	u1Byte						*pLen = NULL;
	
	FrameBuf_Add_u1(pBuf, (u1Byte)EID_Vendor);

	pLen = FrameBuf_Add(pBuf, 1);
		
	FrameBuf_Add_be_u4(pBuf, P2P_IE_VENDOR_TYPE);

	return pLen;
}

VOID
p2p_update_IeHdrLen(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					*pLen
	)
{
	FRAME_BUF					attrBuf;
	FRAME_BUF					*pDupAttrBuf = NULL;
	u2Byte						attrLen = (u2Byte)(FrameBuf_Tail(pBuf) - 1 - pLen);

	if(attrLen <= P2P_BUILD_MAX_ATTR_LEN_PER_IE)
	{
		p2p_write_IeLength(pBuf, pLen);
	return;
	}
		else
	{
		FrameBuf_Init(attrLen, attrLen, pLen + 1, &attrBuf);

		pDupAttrBuf = FrameBuf_Clone(&attrBuf);

		if(!pDupAttrBuf) return;

		FrameBuf_Minus(pBuf, P2P_BUILD_P2P_IE_HDR_LEN);

		p2p_build_AttrIe(pBuf, pDupAttrBuf);

		FrameBuf_Free(pDupAttrBuf);
	}
	
	return;
}

VOID



p2p_add_ActionFrameMacHdr(
	IN  FRAME_BUF				*pBuf,
	IN  const u1Byte			*dest,
	IN  const u1Byte			*src,
	IN  const u1Byte			*bssid
	)
{
	pu1Byte						buf = NULL;
	
	if(NULL == (buf = FrameBuf_Add(pBuf, 24))) return;
	
	SET_80211_HDR_FRAME_CONTROL(buf, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(buf, Type_Action);
	SET_80211_HDR_DURATION(buf, 0);
	SET_80211_HDR_ADDRESS1(buf, dest);
	SET_80211_HDR_ADDRESS2(buf, src);
	SET_80211_HDR_ADDRESS3(buf, bssid);
	SET_80211_HDR_FRAGMENT_SEQUENCE(buf, 0);

	return;
}

VOID
p2p_add_MgntFrameMacHdr(
	IN  FRAME_BUF				*pBuf,
	IN  TYPE_SUBTYPE			typeSubtype,
	IN  const u1Byte			*dest,
	IN  const u1Byte			*src,
	IN  const u1Byte			*bssid
	)
{
	pu1Byte						buf = NULL;
	
	if(NULL == (buf = FrameBuf_Add(pBuf, 24))) return;
	
	SET_80211_HDR_FRAME_CONTROL(buf, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(buf, typeSubtype);
	SET_80211_HDR_DURATION(buf, 0);
	
	SET_80211_HDR_FRAGMENT_SEQUENCE(buf, 0);
	SET_80211_HDR_ADDRESS1(buf, dest);
	SET_80211_HDR_ADDRESS2(buf, src);
	SET_80211_HDR_ADDRESS3(buf, bssid);
	SET_80211_HDR_FRAGMENT_SEQUENCE(buf, 0);

	return;
}

VOID
p2p_add_PublicActionHdr(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					actionCode,
	IN  u1Byte					dialogToken
	)
{
	FrameBuf_Add_u1(pBuf, WLAN_ACTION_PUBLIC);
	FrameBuf_Add_u1(pBuf, actionCode);
	FrameBuf_Add_u1(pBuf, dialogToken);

	return;
}

#endif
