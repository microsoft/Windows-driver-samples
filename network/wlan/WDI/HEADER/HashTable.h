//-----------------------------------------------------------------------------
//	File:
//		HashTable.h
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

#ifndef __INC_HASH_TABLE_H
#define __INC_HASH_TABLE_H

#define RT_HASH_KEY unsigned char*

//
//	Description:
//		Return a index in [0,Capacity-1] from given key.
//
typedef unsigned int
(*RT_HT_HASH_FUNC)(
	IN RT_HASH_KEY			Key
	);


//
// Definition of a hash entry of an value object.
//
typedef struct _RT_HASH_ENTRY{
	RT_LIST_ENTRY			BusyLink; // For list of all value objects in the hash table. 
	RT_LIST_ENTRY			BucketLink; // For list of value objects in the same bucket.
	RT_SINGLE_LIST_ENTRY	FreeLink; // For list of free objects in the hash table.
	RT_HASH_KEY				Key; // Key associated.
}RT_HASH_ENTRY, *PRT_HASH_ENTRY;

//
// This macro must be included in first line of data structure definition of a value object.
//
#define DECLARE_RT_HASH_ENTRY RT_HASH_ENTRY __HashEntry

//
// Routines to translate from link list entry inside RT_HASH_ENTRY object 
// to pointer to the RT_HASH_ENTRY object.
//
#define RT_HASH_ENTRY_FROM_BUSY_LINK(__pBusyLink) (PRT_HASH_ENTRY)(__pBusyLink)
#define RT_HASH_ENTRY_FROM_BUCKET_LINK(__pBucketLink) (PRT_HASH_ENTRY)( (pu1Byte)(__pBucketLink) - sizeof(RT_LIST_ENTRY) )
#define RT_HASH_ENTRY_FROM_FREE_LINK(__pFreeLink) (PRT_HASH_ENTRY)( (pu1Byte)(__pFreeLink) - (sizeof(RT_LIST_ENTRY)*2) )

//
// Definition of the hash table.
//
typedef struct _RT_HASH_TABLE {
	//
	// Value object pool.
	//
	unsigned int		NumValuesAlloc; // Number of value objects allcoated in pValuesPool.
	unsigned int		ValueSize; // # bytes of a value object.
	void*				pValuesBuf; // Pointer to the buffer allocated for value objects.
	unsigned int		KeySize; // # bytes of the key.
	void*				pKeysBuf; // Pointer to the buffer to store key of each value object.
	RT_SINGLE_LIST_HEAD	FreeValuesList; // List of available value object.

	//
	// Hash table stuff.
	//
	RT_HT_HASH_FUNC		pfHash; // Hash function. 
	RT_LIST_ENTRY		BusyValuesList; // List of all value object put in Buckets[].
	unsigned int 		Capacity; // Number of Buckets[] allocated.
	RT_LIST_ENTRY 		Buckets[1]; // Each entry accommodates value objects of the same hash result. 
}*RT_HASH_TABLE_HANDLE;


//
// Hash Table Operations.
//
RT_HASH_TABLE_HANDLE
RtAllocateHashTable(
	IN void*				Adapter,
	IN unsigned int			Capacity,
	IN unsigned int			ValueSize,
	IN unsigned int			KeySize,
	IN RT_HT_HASH_FUNC		pfHash
	);

void
RtFreeHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable
	);

void
RtResetHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable
	);

PRT_HASH_ENTRY
RtPutKeyToHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable,
	IN RT_HASH_KEY			Key
	);

void
RtRemvoeKeyFromVaHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable,
	IN RT_HASH_KEY			Key
	);

PRT_HASH_ENTRY
RtGetValueFromHashTable(
	IN RT_HASH_TABLE_HANDLE	hHashTable,
	IN RT_HASH_KEY			Key
	);

#define RtGetAllValuesFromHashTable(__hHashTable) &((__hHashTable)->BusyValuesList) 

#endif

