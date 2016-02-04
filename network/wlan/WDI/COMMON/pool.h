//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_POOL_H
#define __INC_POOL_H


//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
typedef struct _POOL
{
	const char				*sig;
	const char				*name;
	const u1Byte			*start;
	const u1Byte			*end;
	u4Byte					entrySize;
	RT_LIST_ENTRY			freeList;
	u4Byte					freeCount;
	u4Byte					cap;
	u4Byte					lowMark;

	u8Byte					dbgComp;
	u4Byte					dbgLevel;
}POOL;

//-----------------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------------

VOID
Pool_Init(
	IN  POOL					*pool,
	IN  char					*name,
	IN  u4Byte					buflen,
	IN  VOID					*buf,
	IN  u4Byte					entrySize,
	IN  u8Byte					dbgComp,
	IN  u4Byte					dbgLevel
	);

VOID *
Pool_Acquire(
	IN  POOL					*pool
	);

VOID
Pool_Release(
	IN  POOL					*pool,
	IN  VOID					*entry
	);

VOID
Pool_Dump(
	IN  const POOL				*pool
	);

#endif	// #ifndef __INC_POOL_H