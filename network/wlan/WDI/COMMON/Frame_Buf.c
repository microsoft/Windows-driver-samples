#include "Mp_Precomp.h"

#include "Frame_Buf.h"

#define FRAME_BUF_DEFAULT_DBG_LEVEL DBG_TRACE

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

static const char *framebuf_sig = "FRAME_BUF";

static
BOOLEAN
framebuf_Initialized(
	IN  const FRAME_BUF			*pBuf
	)
{
	return (char *)pBuf->sig == framebuf_sig;
}

static
VOID
framebuf_Init(
	IN  u2Byte					cap,
	IN  u2Byte					len,
	IN  u1Byte					*buf,
	IN  u4Byte					flags,
	IN  u4Byte					dbgLevel,
	OUT FRAME_BUF				*pBuf
	)
{
	pBuf->sig = (u1Byte *)framebuf_sig;
	pBuf->cap = cap;
	pBuf->os.Length = len;
	pBuf->os.Octet = buf;
	pBuf->flags = flags;
	pBuf->dbgLevel = dbgLevel;
	
	return;
}

static
pu1Byte
framebuf_Add(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					size
	)
{
	u1Byte						*pData = FrameBuf_MTail(pBuf);

	RT_ASSERT(framebuf_Initialized(pBuf), ("%s(): accessing uninitialized frame buffer\n", __FUNCTION__));
	
	if(pBuf->cap < pBuf->os.Length + size)
	{// overflow
		return NULL;
	}

	pBuf->os.Length += size;
	
	return pData;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

const u1Byte *
FrameBuf_Head(
	IN  const FRAME_BUF			*pBuf
	)
{
	return pBuf->os.Octet;
}

u1Byte *
FrameBuf_MHead(
	IN  FRAME_BUF				*pBuf
	)
{
	return pBuf->os.Octet;
}

const u1Byte *
FrameBuf_Tail(
	IN  const FRAME_BUF			*pBuf
	)
{
	return pBuf->os.Octet + pBuf->os.Length;
}

u1Byte *
FrameBuf_MTail(
	IN  FRAME_BUF				*pBuf
	)
{
	return pBuf->os.Octet + pBuf->os.Length;
}

u2Byte
FrameBuf_Length(
	IN  const FRAME_BUF			*pBuf
	)
{
	return pBuf->os.Length;
}

u2Byte
FrameBuf_TailRoom(
	IN  const FRAME_BUF			*pBuf
	)
{
	return pBuf->cap - pBuf->os.Length;
}

VOID
FrameBuf_Dump(
	IN  const FRAME_BUF			*pBuf,
	IN  u8Byte					dbgComp,
	IN  u4Byte					dbgLevel,
	IN  const char *			strTitle
	)
{
	RT_TRACE(dbgComp, dbgLevel, ("%s()\n", strTitle));
	RT_PRINT_DATA(dbgComp, dbgLevel, "", FrameBuf_Head(pBuf), FrameBuf_Length(pBuf));

	return;
}

VOID
FrameBuf_DumpFrom(
	IN  const FRAME_BUF			*pBuf,
	IN  const u1Byte			*pHead,
	IN  u8Byte					dbgComp,
	IN  u4Byte					dbgLevel,
	IN  const char *			strTitle
	)
{
	const u1Byte				*pos, *end;

	pos = pHead;
	end = FrameBuf_Head(pBuf) + FrameBuf_Length(pBuf);

	RT_ASSERT(pHead < end, ("%s(): invalid pHead: %p\n", __FUNCTION__, pHead));

	RT_TRACE(dbgComp, dbgLevel, ("%s()\n", strTitle));
	RT_PRINT_DATA(dbgComp, dbgLevel, "", pos, (end - pos));

	return;
}


VOID
FrameBuf_Init(
	IN  u2Byte					cap,
	IN  u2Byte					len,
	IN  u1Byte					*buf,
	OUT FRAME_BUF				*pBuf
	)
{
	framebuf_Init(cap, len, buf, FRAME_BUF_FLAG_EXTERNAL_BUF, FRAME_BUF_DEFAULT_DBG_LEVEL, pBuf);

	return;
}

FRAME_BUF *
FrameBuf_Alloc(
	IN  u2Byte					cap
	)
{
	FRAME_BUF					*pBuf = NULL;
	u4Byte						allocSize = sizeof(FRAME_BUF) + cap;

	PlatformAllocateMemory(NULL, (PVOID *)&pBuf, allocSize);

	if(!pBuf) return NULL;

	framebuf_Init(cap, 0, (pu1Byte)pBuf + sizeof(FRAME_BUF), 0, FRAME_BUF_DEFAULT_DBG_LEVEL, pBuf);

	return pBuf;
}

VOID
FrameBuf_Free(
	IN  FRAME_BUF				*pBuf
	)
{
	if(TEST_FLAG(pBuf->flags, FRAME_BUF_FLAG_EXTERNAL_BUF))
	{// we are not responsible for external buf
		return;
	}
	
	PlatformFreeMemory(pBuf, sizeof(FRAME_BUF) + pBuf->cap);

	return;
}

FRAME_BUF *
FrameBuf_Clone(
	IN  const FRAME_BUF			*pBuf
	)
{
	FRAME_BUF					*pDestBuf = NULL;

	if(NULL == (pDestBuf = FrameBuf_Alloc(pBuf->cap)))
	{
		return NULL;
	}

	FrameBuf_Add_Data(pDestBuf, FrameBuf_Head(pBuf), FrameBuf_Length(pBuf));

	return pDestBuf;
}

BOOLEAN
FrameBuf_Append(
	IN  FRAME_BUF				*pDest,
	IN  const FRAME_BUF			*pSrc
	)
{
	return FrameBuf_Add_Data(pDest, FrameBuf_Head(pSrc), FrameBuf_Length(pSrc));
}

u1Byte *
FrameBuf_Add(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					size
	)
{
	return framebuf_Add(pBuf, size);
}

u1Byte *
FrameBuf_Minus(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					size
	)
{
	if(pBuf->os.Length < size)
	{
		return NULL;
	}

	pBuf->os.Length -= size;

	return pBuf->os.Octet + pBuf->os.Length;
}

BOOLEAN
FrameBuf_Add_u1(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					data
	)
{
	pu1Byte						pData = framebuf_Add(pBuf, 1);

	if(!pData) return FALSE;

	WriteEF1Byte(pData, data);
	
	return TRUE;
}

BOOLEAN
FrameBuf_Add_le_u2(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					data
	)
{
	pu1Byte						pData = framebuf_Add(pBuf, 2);

	if(!pData) return FALSE;
	
	WriteEF2Byte(pData, data);
	
	return TRUE;
}

BOOLEAN
FrameBuf_Add_le_u4(
	IN  FRAME_BUF				*pBuf,
	IN  u4Byte					data
	)
{
	pu1Byte						pData = framebuf_Add(pBuf, 4);

	if(!pData) return FALSE;
	
	WriteEF4Byte(pData, data);
	
	return TRUE;
}

BOOLEAN
FrameBuf_Add_be_u2(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					data
	)
{
	pu1Byte						pData = framebuf_Add(pBuf, 2);

	if(!pData) return FALSE;
	
	WriteH2N2BYTE(pData, data);

	return TRUE;
}

BOOLEAN
FrameBuf_Add_be_u4(
	IN  FRAME_BUF				*pBuf,
	IN  u4Byte					data
	)
{
	pu1Byte						pData = framebuf_Add(pBuf, 4);

	if(!pData) return FALSE;
	
	WriteH2N4BYTE(pData, data);
	
	return TRUE;
}

BOOLEAN
FrameBuf_Add_Data(
	IN  FRAME_BUF				*pBuf,
	IN  const VOID				*data,
	IN  u2Byte					len
	)
{
	pu1Byte						pData = framebuf_Add(pBuf, len);
	u2Byte						it = 0;

	if(!pData) return FALSE;

	// Not to use PlatformMoveMemory because it dosen't declare src as const
	for(it = 0; it < len; it++) pData[it] = ((u1Byte *)data)[it];
		
	return TRUE;
}

u4Byte
FrameBuf_Flags(
	IN  const FRAME_BUF			*pBuf
	)
{
	return pBuf->flags;
}

u2Byte
FrameBuf_Cap(
	IN  const FRAME_BUF			*pBuf
	)
{
	return pBuf->cap;
}

u4Byte
FrameBuf_DbgLevel(
	IN  const FRAME_BUF			*pBuf
	)
{
	return pBuf->dbgLevel;
}

VOID
FrameBuf_SetDbgLevel(
	IN  FRAME_BUF				*pBuf,
	IN  u4Byte					dbgLevel
	)
{
	pBuf->dbgLevel = dbgLevel;
}
