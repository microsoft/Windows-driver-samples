//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_FRAME_BUF_H
#define __INC_FRAME_BUF_H

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------

// Capacity
#define FRAME_BUF_CAP_UNKNOWN 0xFFFF

// Flags
#define FRAME_BUF_FLAG_EXTERNAL_BUF		BIT0

// The internal structure the exported functions operat on.
typedef struct _FRAME_BUF
{
	const u1Byte				*sig;		// signature
	OCTET_STRING				os;			// the buf
	u2Byte						cap; 		// capacity
	u4Byte						flags;		// flags
	u4Byte						dbgLevel; 	// debug level
}FRAME_BUF;

//-----------------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------------

const u1Byte *
FrameBuf_Head(
	IN  const FRAME_BUF			*pBuf
	);

u1Byte *
FrameBuf_MHead(
	IN  FRAME_BUF				*pBuf
	);

const u1Byte *
FrameBuf_Tail(
	IN  const FRAME_BUF			*pBuf
	);

u1Byte *
FrameBuf_MTail(
	IN  FRAME_BUF				*pBuf
	);

u2Byte
FrameBuf_Length(
	IN  const FRAME_BUF			*pBuf
	);

u2Byte
FrameBuf_TailRoom(
	IN  const FRAME_BUF			*pBuf
	);

VOID
FrameBuf_Dump(
	IN  const FRAME_BUF			*pBuf,
	IN  u8Byte					dbgComp,
	IN  u4Byte					dbgLevel,
	IN  const char *			strTitle
	);

VOID
FrameBuf_DumpFrom(
	IN  const FRAME_BUF			*pBuf,
	IN  const u1Byte			*pHead,
	IN  u8Byte					dbgComp,
	IN  u4Byte					dbgLevel,
	IN  const char *			strTitle
	);

VOID
FrameBuf_Init(
	IN  u2Byte					cap,
	IN  u2Byte					len,
	IN  u1Byte					*buf,
	OUT FRAME_BUF				*pBuf
	);

FRAME_BUF *
FrameBuf_Alloc(
	IN  u2Byte					cap
	);

VOID
FrameBuf_Free(
	IN  FRAME_BUF				*pBuf
	);

FRAME_BUF *
FrameBuf_Clone(
	IN  const FRAME_BUF			*pBuf
	);

BOOLEAN
FrameBuf_Append(
	IN  FRAME_BUF				*pDest,
	IN  const FRAME_BUF			*pSrc
	);

u1Byte *
FrameBuf_Add(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					size
	);

u1Byte *
FrameBuf_Minus(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					size
	);

BOOLEAN
FrameBuf_Add_u1(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					data
	);

BOOLEAN
FrameBuf_Add_le_u2(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					data
	);

BOOLEAN
FrameBuf_Add_le_u4(
	IN  FRAME_BUF				*pBuf,
	IN  u4Byte					data
	);

BOOLEAN
FrameBuf_Add_be_u2(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					data
	);

BOOLEAN
FrameBuf_Add_be_u4(
	IN  FRAME_BUF				*pBuf,
	IN  u4Byte					data
	);

BOOLEAN
FrameBuf_Add_Data(
	IN  FRAME_BUF				*pBuf,
	IN  const VOID				*data,
	IN  u2Byte					len
	);

u4Byte
FrameBuf_Flags(
	IN  const FRAME_BUF			*pBuf
	);

u2Byte
FrameBuf_Cap(
	IN  const FRAME_BUF			*pBuf
	);

u4Byte
FrameBuf_DbgLevel(
	IN  const FRAME_BUF			*pBuf
	);

VOID
FrameBuf_SetDbgLevel(
	IN  FRAME_BUF				*pBuf,
	IN  u4Byte					dbgLevel
	);

#endif	// #ifndef __INC_FRAME_BUF_H
