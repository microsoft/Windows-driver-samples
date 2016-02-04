#ifndef __INC_DEFRAG_H
#define __INC_DEFRAG_H

#define MaxDefragLifeTime			51200	// in us


VOID
DefragInitialize(
	PADAPTER		Adapter
	);

PRT_RFD
DefragAddRFD(
	PADAPTER		Adapter,
	PRT_RFD			pRfd,
	pu1Byte			pSenderAddr,
	u1Byte			TID,
	u2Byte			SeqNum,
	u1Byte			FragNum,
	BOOLEAN			bMoreFrag,
	BOOLEAN			bEncrypted
	);

VOID
DefragReset(
	PADAPTER		Adapter
	);


/*
 *	Note:	Following functions should not be called outside Defrag.c
 *
*/
VOID
DefragInit(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size
	);

PDEFRAG_ENTRY
DefragSearch(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size,
	pu1Byte			pSenderAddr,
	u1Byte			TID,
	u2Byte			SeqNum,
	u1Byte			FragNum
	);

VOID
DefragRemoveOldest(
	PADAPTER	Adapter
	);

PDEFRAG_ENTRY
DefragFindFreeEntry(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size
	);

VOID
DefragAge(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size,
	u8Byte			usCurrentTime,
	PADAPTER		Adapter
	);

VOID
DefragEntrySetRFD(
	PDEFRAG_ENTRY	pEntry,
	PRT_RFD			pRfd,
	pu1Byte			pSenderAddr,
	u1Byte			TID,
	u2Byte			SeqNum,
	u1Byte			FragNum	
	);

VOID
DefragEntryAddRFD(
	PDEFRAG_ENTRY	pEntry,
	PRT_RFD			pRfd,
	u1Byte			FragNum	
	);

VOID
DefragEntryFree(
	PDEFRAG_ENTRY	pEntry,
	PADAPTER		Adapter	
	);

VOID
DefragRecycleRFD(
	PADAPTER		Adapter
	);

VOID
DefragFreeLRUEntry(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size,
	PADAPTER		Adapter
	);

#endif // #ifndef __INC_DEFRAG_H
