#ifndef __INC_OBJECT_H
#define __INC_OBJECT_H


typedef struct _RT_OBJECT{
	RT_LIST_ENTRY	ListEntry;
	void*			pStartVa;
	unsigned int	SizeAlloc;
}RT_OBJECT, *PRT_OBJECT;

#define DECLARE_RT_OBJECT(__DATA_TYPE) RT_OBJECT __Object

#define INIT_RT_OBJECT_LIST(__pObjectList) RTInitializeListHead(__pObjectList);


#define ADD_RT_OBJECT(__pObjectList, __pPtr, __DATA_TYPE) \
	{ \
		PRT_OBJECT pObject = &(((__DATA_TYPE *)(__pPtr))->__Object); \
		pObject->pStartVa = (__pPtr); \
		pObject->SizeAlloc = sizeof(__DATA_TYPE); \
		RTInsertTailList(__pObjectList, &(pObject->ListEntry) ); \
	}

#define REMOVE_RT_OBJECT(__pObjectList) RTRemoveHeadList(__pObjectList)

#define HAS_RT_OBJECT(__pObjectList) RTIsListNotEmpty(__pObjectList)

#define ALLOC_RT_OBJECT(__pAdapter, __pObjectList, __ppOutPtr, __DATA_TYPE) \
	{ \
		*(__ppOutPtr) = NULL; \
		if( RT_STATUS_SUCCESS == PlatformAllocateMemory((__pAdapter), (__ppOutPtr),  sizeof(__DATA_TYPE)) ) { \
			PlatformZeroMemory(*(__ppOutPtr), sizeof(__DATA_TYPE)); \
			ADD_RT_OBJECT(__pObjectList, *(__ppOutPtr), __DATA_TYPE); \
		} \
	}

#define FREE_RT_OBJECT(__pObjectList) \
	{ \
		PRT_OBJECT __pObject = (PRT_OBJECT)REMOVE_RT_OBJECT(__pObjectList); \
		PlatformFreeMemory((__pObject)->pStartVa, (__pObject)->SizeAlloc); \
	}

#endif
