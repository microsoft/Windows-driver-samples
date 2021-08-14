/******************************Module*Header*******************************\
* Module Name: bdd.h
*
* Basic Display Driver memory allocation, deletion, and tracking 
*
*
* Copyright (c) 2010 Microsoft Corporation
\**************************************************************************/

#include "BDD.hxx"

#pragma code_seg("PAGE")

//
// New and delete operators
//
void* __cdecl operator new(size_t Size, BDD_POOL_FLAGS PoolFlags)
{
    PAGED_CODE();

    Size = (Size != 0) ? Size : 1;
    
    void* pObject = ExAllocatePool2(static_cast<POOL_FLAGS>(PoolFlags), Size, BDDTAG);

#if DBG
    if (pObject != NULL)
    {
        RtlFillMemory(pObject, Size, 0xCD);
    }
#endif // DBG

    return pObject;
}

void* __cdecl operator new[](size_t Size, BDD_POOL_FLAGS PoolFlags)
{
    PAGED_CODE();

    Size = (Size != 0) ? Size : 1;
    
    void* pObject = ExAllocatePool2(static_cast<POOL_FLAGS>(PoolFlags), Size, BDDTAG);

#if DBG
    if (pObject != NULL)
    {
        RtlFillMemory(pObject, Size, 0xCD);
    }
#endif // DBG

    return pObject;
}

void __cdecl operator delete(void* pObject)
{
    PAGED_CODE();

    if (pObject != NULL)
    {
        ExFreePool(pObject);
    }
}

//
// size_t version is needed for VS2015(C++ 14).  
// 
void __cdecl operator delete(void* pObject, size_t s)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( s );

    ::operator delete( pObject );
}

void __cdecl operator delete[](void* pObject)
{
    PAGED_CODE();

    if (pObject != NULL)
    {
        ExFreePool(pObject);
    }
}

