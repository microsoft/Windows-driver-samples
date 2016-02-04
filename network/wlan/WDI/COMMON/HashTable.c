//-----------------------------------------------------------------------------
//	File:
//		HashTable.c
//
//	Description:
//		Generic hash table for kernel mode module.
//
//	Note:
//		1. Memory allocation is a issue on kernel space for calling context 
//		and computation time limitation. So we don't re-hash the table even 
//		it is full. In short, this implementation is not a perfect hash, 
//		however, if user has a good guess on hash table usage to prevent 
//		full and colission condition, this implementation is still a 
//		constant time on put, remove, and get operations.
//
//		2. This implementation is not thread-safe, that is, user have to 
//		protect related resource and the function exported here by their 
//		own means.
//		
//	070606, by rcnjko.
//-----------------------------------------------------------------------------

#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "HashTable.tmh"
#endif

typedef struct _RT_HASH_TABLE RT_HASH_TABLE;

//================================================================================
//	Prototype of protected function.
//================================================================================
int 
RtCompareKeys(
	IN pu1Byte		Key1,
	IN pu1Byte		Key2,
	IN u4Byte		KeySize
	);
//================================================================================


//
// Description;
//	Reset hash table to initialized state.
//
void
RtResetHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable
	)
{
	PRT_LIST_ENTRY		pTmpListEntry;
	PRT_HASH_ENTRY		pHashEntry;

	while( RTIsListNotEmpty(&(hHashTable->BusyValuesList)) )
	{
		pTmpListEntry = RTRemoveHeadList(&(hHashTable->BusyValuesList));
		pHashEntry = RT_HASH_ENTRY_FROM_BUSY_LINK( pTmpListEntry );

		RTRemoveEntryList( &(pHashEntry->BucketLink) );
		RTInsertTailSList( &(hHashTable->FreeValuesList), &(pHashEntry->FreeLink) );
	}

#if DBG
	{
		PRT_SINGLE_LIST_ENTRY pTmpSListEntry;
		u4Byte				idx;

		RT_ASSERT( RTIsListEmpty(&(hHashTable->BusyValuesList)), ("hHashTable(%p) BusyValuesList(%p) should be empty!!!\n", hHashTable, &(hHashTable->BusyValuesList) ));
		for(idx = 0; idx < hHashTable->Capacity; idx++)
		{
			RT_ASSERT( RTIsListEmpty(&(hHashTable->Buckets[idx])), ("hHashTable(%p) Buckets[%d]:%p should be empty!!!\n", hHashTable, idx, &(hHashTable->Buckets[idx])));
		}

		idx = 0;
		for(pTmpSListEntry = RTGetHeadSList( &(hHashTable->FreeValuesList) ); 
			pTmpSListEntry != NULL && idx < hHashTable->Capacity; 
			pTmpSListEntry = pTmpSListEntry->Next)
		{
			pHashEntry = RT_HASH_ENTRY_FROM_FREE_LINK(pTmpSListEntry);

			RT_ASSERT( ((pu1Byte)pHashEntry >= (pu1Byte)(hHashTable->pValuesBuf) && 
						(pu1Byte)pHashEntry < ((pu1Byte)(hHashTable->pValuesBuf) + (hHashTable->ValueSize * hHashTable->NumValuesAlloc))), 
						("hHashTable(%p) idx(%d) pHashEntry(%p): invalid pHashEntry!!!\n",
						hHashTable, idx, pHashEntry));
			idx++;
		}
		RT_ASSERT(idx == hHashTable->NumValuesAlloc, 
			("hHashTable(%p) idx(%d) != NumValuesAlloc(%d), pTmpSListEntry(%p)\n", 
			hHashTable, idx, hHashTable->NumValuesAlloc, pTmpSListEntry));
		RT_ASSERT(pTmpSListEntry == NULL, 
			("hHashTable(%p) pTmpSListEntry(%p) != NULL, idx(%d), NumValuesAlloc(%d)\n", 
			hHashTable, pTmpSListEntry, idx, hHashTable->NumValuesAlloc));
	}
#endif
}

//
//	Description:
//		Allocate memory for hast table and value object pool.
//		It will reset hash table to initial state for further operation.
//
//	Input:
//		Capacity: number of bucket of the hash table to allocated.
//		ValueSize: number of byte of a value object.
//		KeySize: number of byte of the key. 
//		pfHash: pointer to the hash function, see definition of RT_HT_HASH_FUNC for detail.
//
//	Output:
//		Return handle of a hash table if succeeded, NULL otherwise.
//
//	Note:
//		1. The number of value objects allocated might be less than NumValuesAlloc.
//
//	Assumption:
//		1. In the context to invoke PlatformAllocateMemory().
//
RT_HASH_TABLE_HANDLE
RtAllocateHashTable(
	IN void*				Adapter,
	IN unsigned int			Capacity,
	IN unsigned int			ValueSize,
	IN unsigned int			KeySize,
	IN RT_HT_HASH_FUNC		pfHash
	)
{
	PADAPTER				pAdapter = (PADAPTER)Adapter;
	RT_STATUS				rtStatus;
	RT_HASH_TABLE_HANDLE	pTable = NULL;
	u4Byte					TableSize;
	u4Byte					NumValuesAlloc = Capacity;
	pu1Byte					pValuesBuf = NULL;
	u4Byte					ValuesBufSize=0;
	pu1Byte					pKeysBuf = NULL;
	u4Byte					KeysBufSize=0;
	u4Byte					idx;
	PRT_HASH_ENTRY			pHashEntry;
	pu1Byte					pKey;

	RT_TRACE(COMP_INIT, DBG_TRACE, ("RtAllocateHashTable(): Capacity(%d), NumValuesAlloc(%d), ValueSize(%d)\n",
		Capacity, NumValuesAlloc, ValueSize));

	do {
		//
		// Allocate memory for hash table.
		//
		TableSize = sizeof(RT_HASH_TABLE) + ((Capacity - 1) * sizeof(RT_LIST_ENTRY));
		rtStatus = PlatformAllocateMemory(pAdapter, (PVOID*)(&pTable), TableSize);
		if( RT_STATUS_SUCCESS != rtStatus )
		{
			RT_ASSERT(FALSE, ("RtAllocateHashTable(): failed to allocate TableSize(%d) !!!\n", TableSize));
			break;
		}
		PlatformZeroMemory(pTable, TableSize);
		RT_TRACE(COMP_INIT, DBG_TRACE, ("RtAllocateHashTable(): table: %p size: %d\n", pTable, TableSize));

		//
		// Allocate memory for value object pool.
		//
		ValuesBufSize = NumValuesAlloc * ValueSize;
		rtStatus = PlatformAllocateMemory(pAdapter, (PVOID*)(&pValuesBuf), ValuesBufSize);
		if( RT_STATUS_SUCCESS != rtStatus )
		{
			RT_ASSERT(FALSE, ("RtAllocateHashTable(): failed to allocate value objects, NumValuesAlloc(%d), ValueSize(%d)!!!\n", NumValuesAlloc, ValueSize));
			break;
		}
		PlatformZeroMemory(pValuesBuf, ValuesBufSize);
		RT_TRACE(COMP_INIT, DBG_TRACE, ("RtAllocateHashTable(): pValueBuf: %p size: %d\n", pValuesBuf, ValuesBufSize));

		//
		// Allocate memory for keys.
		//
		KeysBufSize = NumValuesAlloc * KeySize;
		rtStatus = PlatformAllocateMemory(pAdapter, (PVOID*)(&pKeysBuf), KeysBufSize);
		if( RT_STATUS_SUCCESS != rtStatus )
		{
			RT_ASSERT(FALSE, ("RtAllocateHashTable(): failed to allocate keys, NumValuesAlloc(%d), KeySize(%d)!!!\n", NumValuesAlloc, KeySize));
			break;
		}
		PlatformZeroMemory(pKeysBuf, KeysBufSize);
		RT_TRACE(COMP_INIT, DBG_TRACE, ("RtAllocateHashTable(): pKeysBuf: %p size: %d\n", pKeysBuf, KeysBufSize));

		//
		// Initialize value object pool stuff
		//
		pTable->NumValuesAlloc = NumValuesAlloc;
		pTable->ValueSize = ValueSize;
		pTable->pValuesBuf = pValuesBuf;
		pTable->KeySize = KeySize;
		pTable->pKeysBuf = pKeysBuf;
		RTInitializeSListHead( &(pTable->FreeValuesList) );

		pHashEntry = (PRT_HASH_ENTRY)pValuesBuf;
		pKey = (pu1Byte)pKeysBuf;
		for(idx = 0; idx < NumValuesAlloc; idx++)
		{
			pHashEntry->Key = pKey;
			RTInsertTailSList(&(pTable->FreeValuesList), &(pHashEntry->FreeLink));
			
			pHashEntry = (PRT_HASH_ENTRY)((pu1Byte)pHashEntry + ValueSize);
			pKey = (pu1Byte)pKey + KeySize;
		}

		//
		// Initialize hash table stuff.
		//
		pTable->pfHash = pfHash;
		RTInitializeListHead( &(pTable->BusyValuesList) );
		pTable->Capacity = Capacity;
		for(idx = 0; idx < Capacity; idx++)
		{
			RTInitializeListHead( &(pTable->Buckets[idx]) );
		}

		//
		// Return the hash table allocated.
		//
		return pTable;

	}while(FALSE);
	
	//
	// Error case.
	//
	if(pTable != NULL)
		PlatformFreeMemory(pTable, TableSize);

	if(pValuesBuf != NULL)
		PlatformFreeMemory(pValuesBuf, ValuesBufSize);

	if(pKeysBuf != NULL)
		PlatformFreeMemory(pKeysBuf, KeysBufSize);


	return NULL;
}

//
//	Description:
//		Return the value object of specified key if found, 
//		NULL otherwise.
//
PRT_HASH_ENTRY
RtGetValueFromHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable,
	IN RT_HASH_KEY			Key
	)
{
	PRT_HASH_ENTRY		pHashEntry = NULL;
	PRT_LIST_ENTRY		pTmpListEntry;
	u4Byte				idx;

	idx = hHashTable->pfHash(Key);
	for( pTmpListEntry = hHashTable->Buckets[idx].Flink;
		 pTmpListEntry != &(hHashTable->Buckets[idx]); 
		 pTmpListEntry = pTmpListEntry->Flink )
	{
		pHashEntry = RT_HASH_ENTRY_FROM_BUCKET_LINK(pTmpListEntry);
		// Compare the keys if they are equal.
		if( RtCompareKeys(Key, pHashEntry->Key, hHashTable->KeySize) == 0 )
		{ 
			return pHashEntry;
		}
	}	

	return NULL;
}

//
// Description:
//	Compare the two keys and return 0 if they are equal, otherwise return an interger indicating the difference.
//
int 
RtCompareKeys(
	IN pu1Byte		Key1,
	IN pu1Byte		Key2,
	IN u4Byte		KeySize
	)
{
	u4Byte		idx;
	int			result = 0;

	for(idx = 0; idx < KeySize; idx++)
	{
		result = Key1[idx] - Key2[idx];
		if(result != 0)
			return result;
	}

	return result;
}

//
//	Description:
//		Retrive an value object from pool and put it to the hash table 
//		according to hash value of the key.
//
//	Output:
//		Return the value object assocaited with the key specfied, 
//		NULL if no available value object now.
//
PRT_HASH_ENTRY
RtPutKeyToHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable,
	IN RT_HASH_KEY			Key
	)
{
	PRT_HASH_ENTRY			pHashEntry = NULL;
	PRT_SINGLE_LIST_ENTRY	pTmpSListEntry;
	u4Byte					idx;

	//
	// Check if Key had existed. if yse, return previous entry.
	//
	if((pHashEntry = RtGetValueFromHashTable(hHashTable, Key)) != NULL)
	{
		return pHashEntry;
	}
	
	//
	// Retrive an value object from pool for a new Key, 
	// and put it into hash table.
	//
	idx = hHashTable->pfHash(Key);
	if( !RTIsSListEmpty(&(hHashTable->FreeValuesList)) )
	{
		pTmpSListEntry = RTRemoveHeadSList(&(hHashTable->FreeValuesList));
		pHashEntry = RT_HASH_ENTRY_FROM_FREE_LINK(pTmpSListEntry);

		PlatformMoveMemory(pHashEntry->Key, Key, hHashTable->KeySize);
		RTInsertTailList(&(hHashTable->BusyValuesList), &(pHashEntry->BusyLink));
		RTInsertTailList(&(hHashTable->Buckets[idx]), &(pHashEntry->BucketLink));
	}

	return pHashEntry;
}

//
//	Description:
//		Free resource allocated in RtAllocateHashTable().
//
//	Assumption:
//		1. In the context to invoke PlatformFreeMemory().
//
void
RtFreeHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable
	)
{
	u4Byte		TableSize;
	u4Byte		ValuesBufSize;
	u4Byte		KeysBufSize;

	if(hHashTable != NULL)
	{
		if(hHashTable->pKeysBuf != NULL)
		{
			KeysBufSize = hHashTable->NumValuesAlloc * hHashTable->KeySize;
			RT_TRACE(COMP_INIT, DBG_TRACE, ("RtFreeHashTable(): pKeysBuf: %p, KeysBufSize: %d\n", hHashTable->pKeysBuf, KeysBufSize));
			PlatformFreeMemory(hHashTable->pKeysBuf, KeysBufSize);
		}

		if(hHashTable->pValuesBuf != NULL)
		{
			ValuesBufSize = hHashTable->NumValuesAlloc * hHashTable->ValueSize;
			RT_TRACE(COMP_INIT, DBG_TRACE, ("RtFreeHashTable(): pValuesBuf: %p, ValuesBufSize: %d\n", hHashTable->pValuesBuf, ValuesBufSize));
			PlatformFreeMemory(hHashTable->pValuesBuf, ValuesBufSize);
		}
	
		TableSize = sizeof(RT_HASH_TABLE) + ((hHashTable->Capacity - 1) * sizeof(RT_LIST_ENTRY));
		RT_TRACE(COMP_INIT, DBG_TRACE, ("RtFreeHashTable(): hHashTable: %p, TableSize: %d\n", hHashTable, TableSize));
		PlatformFreeMemory(hHashTable, TableSize);
	}
}

//
//	Description:
//		Remove value object of specified key from hash table. 
//
void
RtRemvoeKeyFromVaHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable,
	IN RT_HASH_KEY			Key
	)
{
	PRT_HASH_ENTRY	pHashEntry = NULL;
	PRT_LIST_ENTRY	pTmpListEntry;
	u4Byte			idx;

	idx = hHashTable->pfHash(Key);
	for( pTmpListEntry = hHashTable->Buckets[idx].Flink;
		 pTmpListEntry != &(hHashTable->Buckets[idx]); 
		 pTmpListEntry = pTmpListEntry->Flink )
	{
		pHashEntry = RT_HASH_ENTRY_FROM_BUCKET_LINK(pTmpListEntry);
		if( RtCompareKeys(Key, pHashEntry->Key, hHashTable->KeySize) == 0 )
		{ 
			RTRemoveEntryList( &(pHashEntry->BucketLink) );
			RTRemoveEntryList( &(pHashEntry->BusyLink) );

			RTInsertTailSList( &(hHashTable->FreeValuesList), &(pHashEntry->FreeLink) );
		}
	}	
}

