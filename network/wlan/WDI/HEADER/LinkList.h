#ifndef __INC_LINKLIST_H
#define __INC_LINKLIST_H

//
//  Doubly linked list structure.  Can be used as either a list head, or
//  as link words.
//
typedef struct _RT_LIST_ENTRY {
	struct _RT_LIST_ENTRY *Flink;
	struct _RT_LIST_ENTRY *Blink;
} RT_LIST_ENTRY, *PRT_LIST_ENTRY;

#define	RTIsUnInitializedListHead(ListHead)	\
	(NULL == (ListHead)->Flink || NULL == (ListHead)->Blink)

#define	RTIsListHead(ListHead, __Entry)	\
	((ListHead) == (__Entry))

//
//  Doubly-linked list manipulation routines.  Implemented as macros
//
#define RTInitializeListHead(ListHead)				\
	((ListHead)->Flink = (ListHead)->Blink = (ListHead))


#define RTIsListEmpty(ListHead)					\
	((ListHead)->Flink == NULL || (ListHead)->Flink == (ListHead))

#define RTIsListNotEmpty(ListHead)					\
	((ListHead)->Flink != (ListHead))

#define RTGetHeadList(ListHead)					\
	((ListHead)->Flink)

#define RTGetTailList(ListHead)					\
	(ListHead)->Blink

#define RTRemoveHeadList(ListHead)				\
	(ListHead)->Flink;							\
	{RTRemoveEntryList((ListHead)->Flink)}


#define RTRemoveTailList(ListHead)				\
	(ListHead)->Blink;							\
	{RTRemoveEntryList((ListHead)->Blink)}


#define RTNextEntryList(Entry)					\
	(Entry)->Flink

#define RTForeEntryList(Entry)					\
	(Entry)->Blink


#define RTRemoveEntryList(Entry)					\
	{											\
		PRT_LIST_ENTRY _EX_Blink;					\
		PRT_LIST_ENTRY _EX_Flink;					\
		_EX_Flink = (Entry)->Flink;					\
		_EX_Blink = (Entry)->Blink;				\
		_EX_Blink->Flink = _EX_Flink;				\
		_EX_Flink->Blink = _EX_Blink;				\
	}


#define RTInsertTailList(ListHead,Entry)			\
	{											\
		PRT_LIST_ENTRY _EX_Blink;					\
		PRT_LIST_ENTRY _EX_ListHead;				\
		_EX_ListHead = (ListHead);					\
		_EX_Blink = _EX_ListHead->Blink;			\
		(Entry)->Flink = _EX_ListHead;				\
		(Entry)->Blink = _EX_Blink;				\
		_EX_Blink->Flink = (Entry);					\
		_EX_ListHead->Blink = (Entry);				\
	}


#define RTInsertHeadList(ListHead,Entry)			\
	{											\
		PRT_LIST_ENTRY _EX_Flink;					\
		PRT_LIST_ENTRY _EX_ListHead;				\
		_EX_ListHead = (ListHead);					\
		_EX_Flink = _EX_ListHead->Flink;			\
		(Entry)->Flink = _EX_Flink;					\
		(Entry)->Blink = _EX_ListHead;				\
		_EX_Flink->Blink = (Entry);					\
		_EX_ListHead->Flink = (Entry);				\
	}


#define RTPopEntryList(ListHead)					\
	(ListHead)->Next;							\
	{											\
		PRT_SINGLE_LIST_ENTRY FirstEntry;			\
		FirstEntry = (ListHead)->Next;				\
		if (FirstEntry != NULL)						\
		{										\
			(ListHead)->Next = FirstEntry->Next;	\
		}										\
	}


#define RTPushEntryList(ListHead,Entry)			\
	(Entry)->Next = (ListHead)->Next;				\
	(ListHead)->Next = (Entry)

// Insert __InsertedEntry before __Entry
#define	RTInsertEntryBeforeEntry(__InsertedEntry,	__Entry)	\
	{											\
		PRT_LIST_ENTRY _EX_Blink;					\
		_EX_Blink = (__Entry)->Blink;				\
		_EX_Blink->Flink = (__InsertedEntry);			\
		(__InsertedEntry)->Blink = _EX_Blink;			\
		(__InsertedEntry)->Flink = (__Entry);			\
		(__Entry)->Blink = (__InsertedEntry);			\
	}

// Insert __InsertedEntry after __Entry
#define	RTInsertEntryAfterEntry(__InsertedEntry,	__Entry)	\
	{											\
		PRT_LIST_ENTRY _EX_Flink;					\
		_EX_Flink = (__Entry)->Flink;				\
		_EX_Flink->Blink = (__InsertedEntry);			\
		(__InsertedEntry)->Flink = _EX_Flink;			\
		(__InsertedEntry)->Blink = (__Entry);			\
		(__Entry)->Flink = (__InsertedEntry);			\
	}

//---------------------------------------------------------------------------
// The following MACROs do the same with the above MACROs except the Cnt parameter.
// Cnt is a pointer to counter which will be modified.

#define RTRemoveHeadListWithCnt(ListHead,Cnt)				\
	(ListHead)->Flink;										\
	RTRemoveEntryListWithCnt((ListHead)->Flink, Cnt)

#define RTRemoveTailListWithCnt(ListHead,Cnt)					\
	(ListHead)->Blink;										\
	RTRemoveEntryListWithCnt((ListHead)->Blink, Cnt)

#define RTRemoveEntryListWithCnt(Entry,Cnt)					\
	RTRemoveEntryList(Entry)								\
	(*Cnt)--;

#define RTInsertTailListWithCnt(ListHead,Entry,Cnt)				\
	RTInsertTailList(ListHead,Entry)							\
	(*Cnt)++;

#define RTInsertHeadListWithCnt(ListHead,Entry,Cnt)			\
	RTInsertHeadList(ListHead,Entry)							\
	(*Cnt)++;

#define RTPopEntryListWithCnt(ListHead,Cnt)					\
	RTPopEntryList(ListHead)									\
	(*Cnt)--;

#define RTPushEntryListWithCnt(ListHead,Entry,Cnt)				\
	RTPushEntryList(ListHead,Entry)							\
	(*Cnt)++;



#define RTSwitchListHead(orgListHead,	newListHead)			\
{															\
	RTNextEntryList(newListHead) = RTNextEntryList(orgListHead);	\
	RTForeEntryList(RTNextEntryList(orgListHead)) = newListHead;	\
	RTNextEntryList(RTForeEntryList(orgListHead)) = newListHead;	\
	RTForeEntryList(newListHead) = RTForeEntryList(orgListHead);	\
	RTInitializeListHead(orgListHead);						\
}	

#define RtEntryListForEach(__pListHead, __itEntry) \
	for((__itEntry) = RTGetHeadList((__pListHead)); \
		(__itEntry) != (__pListHead); \
		(__itEntry) = RTNextEntryList((__itEntry)) \
		)

#define RtEntryListForEachSafe(__pListHead, __itEntry, __n) \
	for((__itEntry) = RTGetHeadList((__pListHead)), \
			(__n) = (__itEntry)->Flink; \
		(__itEntry) != (__pListHead); \
		(__itEntry) = (__n), (__n) = RTNextEntryList((__n)) \
		)

//
//  Singly linked list structure. 
//  Only used as link words.
//
typedef struct _RT_SINGLE_LIST_ENTRY {
	struct _RT_SINGLE_LIST_ENTRY *Next;
} RT_SINGLE_LIST_ENTRY, *PRT_SINGLE_LIST_ENTRY;

//
//  Singly linked list structure. 
//  Only used as link head.
//
typedef struct _RT_SINGLE_LIST_HEAD {
	PRT_SINGLE_LIST_ENTRY First;
	PRT_SINGLE_LIST_ENTRY Last;
} RT_SINGLE_LIST_HEAD, *PRT_SINGLE_LIST_HEAD;



//
//	Single link list routines.
//
#define RTInitializeSListHead(SListHead) \
	((SListHead)->First = (SListHead)->Last = NULL)

#define RTIsSListEmpty(SListHead) \
	((SListHead)->First == NULL)

#define RTGetHeadSList(SListHead) \
	(PRT_SINGLE_LIST_ENTRY)((SListHead)->First)

#define RTGetTailSList(SListHead) \
	(PRT_SINGLE_LIST_ENTRY)((SListHead)->Last)

#define RTRemoveHeadSList(SListHead) \
	RTGetHeadSList(SListHead); \
	RT_ASSERT(!RTIsSListEmpty(SListHead), ("RTRemoveHeadSList(): SList(%p) is empty!!!\n", SListHead)); \
	{ \
		PRT_SINGLE_LIST_ENTRY pToRemove = (SListHead)->First; \
		if((SListHead)->First == (SListHead)->Last) \
		{ \
			(SListHead)->First = (SListHead)->Last = NULL; \
		} \
		else \
		{ \
			(SListHead)->First = pToRemove->Next; \
		} \
		pToRemove->Next = NULL; \
	}

#define RTInsertTailSList(SListHead, SListEntry) \
	{ \
		PRT_SINGLE_LIST_ENTRY pOldTail = (SListHead)->Last; \
		(SListEntry)->Next = NULL; \
		if(RTIsSListEmpty(SListHead)) \
		{ \
			(SListHead)->First = (SListHead)->Last = SListEntry; \
		} \
		else \
		{ \
			pOldTail->Next = SListEntry; \
			(SListHead)->Last = SListEntry; \
		} \
	}


#endif // #ifndef __INC_LINKLIST_H
