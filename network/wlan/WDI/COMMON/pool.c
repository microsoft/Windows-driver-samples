#include "Mp_Precomp.h"

#include "pool.h"

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

// Signature
static const char *pool_sig = "RT_POOL";

static
BOOLEAN
pool_OnBoundary(
	IN  const POOL				*pool,
	IN  const VOID				*entry
	)
{
	BOOLEAN						bOnBoundary = FALSE;

	do
	{
		if(pool->end <= (u1Byte *)entry)
			break;
		
		if((u1Byte *)entry < pool->start)
			break;
		
		if(((u1Byte *)entry - pool->start) % pool->entrySize)
			break;

		bOnBoundary = TRUE;
		
	}while(FALSE);

	if(!bOnBoundary)
	{
		RT_TRACE_F(pool->dbgComp, DBG_WARNING, ("invalid entry: 0x%p\n", entry));
	}

	return bOnBoundary;
}

static
VOID
pool_DumpListEntry(
	IN  const POOL				*pool,
	IN  const RT_LIST_ENTRY		*pEntry
	)
{
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("---\n"));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("entry: %p\n", pEntry));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("   flink: %p\n", pEntry->Flink));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("   blink: %p\n", pEntry->Blink));
}

static
VOID
pool_DumpFreeList(
	IN  const POOL				*pool
	)
{
	RT_LIST_ENTRY				*pEntry = NULL;

	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("Head:\n"));
	pool_DumpListEntry(pool, &pool->freeList);
	
	for(pEntry = RTGetHeadList(&pool->freeList);
		pEntry != &pool->freeList;
		pEntry = RTNextEntryList(pEntry)
		)
	{
		pool_DumpListEntry(pool, pEntry);
	}
}

//-----------------------------------------------------------------------------
// Exported
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
	)
{
	const u1Byte				*pos = (u1Byte *)buf, *end = (u1Byte *)buf + buflen;

	RT_ASSERT(sizeof(RT_LIST_ENTRY) < entrySize, ("Invalid size: %u\n", entrySize));

	pool->sig = pool_sig;
	pool->name = name;
	pool->start = pos;
	pool->end = end;
	pool->entrySize = entrySize;
	
	RTInitializeListHead(&pool->freeList);
	pool->freeCount = 0;

	while(pos + entrySize <= end)
	{
		RTInsertTailListWithCnt(&pool->freeList, (RT_LIST_ENTRY *)pos, &pool->freeCount);
		pos += entrySize;
		pool->cap++;
	}

	pool->cap = pool->freeCount;
	pool->lowMark = pool->freeCount;

	pool->dbgComp = dbgComp;
	pool->dbgLevel = dbgLevel;

	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("name: %s, cap: %u\n", pool->name, pool->cap));

	return;
}

VOID *
Pool_Acquire(
	IN  POOL					*pool
	)
{
	VOID						*entry = NULL;

	RT_ASSERT(pool, ("%s(): pool is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(pool_sig == pool->sig, ("%s(): invalid pool\n", __FUNCTION__));
	
	if(RTIsListEmpty(&pool->freeList)) return NULL;

	entry = (VOID *)RTRemoveHeadListWithCnt(&pool->freeList, &pool->freeCount);

	if(pool->freeCount < pool->lowMark)
	{
		pool->lowMark = pool->freeCount;
		RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("%s: lowMark: %u, cap: %u\n", pool->name, pool->freeCount, pool->cap));
	}
	
	return entry;
}

VOID
Pool_Release(
	IN  POOL					*pool,
	IN  VOID					*entry
	)
{
	RT_ASSERT(pool, ("%s(): pool is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(pool_sig == pool->sig, ("%s(): invalid pool\n", __FUNCTION__));

	RT_ASSERT(entry < (VOID *)pool->end, ("%s(): entry (%p) beyond end (%p)\n", __FUNCTION__, entry, pool->end));
	RT_ASSERT((VOID *)pool->start <= entry, ("%s(): entry (%p) below start (%p)\n", __FUNCTION__, entry, pool->start));	

	if(pool_OnBoundary(pool, entry))
	{
		RTInsertTailListWithCnt(&pool->freeList, (RT_LIST_ENTRY *)entry, &pool->freeCount);
	}
	else
	{
		Pool_Dump(pool);
	}
	
	return;
}

VOID
Pool_Dump(
	IN  const POOL				*pool
	)
{
	RT_ASSERT(pool, ("%s(): pool is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(pool_sig == pool->sig, ("%s(): invalid pool\n", __FUNCTION__));
	
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("---\n"));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("name: %s\n", pool->name));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("start addr: %p\n", pool->start));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("end addr: %p\n", pool->end));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("free count: %u\n", pool->freeCount));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("capacity: %u\n", pool->cap));
	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("low mark: %u\n", pool->lowMark));

	RT_TRACE_F(pool->dbgComp, pool->dbgLevel, ("Free list:\n"));
	//pool_DumpFreeList(pool);

	return;
}

