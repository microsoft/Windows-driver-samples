// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include <new.h>

#if UMDF_DRIVER == 0
#include <pooltypes.h>
#endif

#include <KDebug.h>
#include <KMacros.h>

//
//                                           KALLOCATOR           KALLOCATOR_NONPAGED
// ---------------------------------------+--------------------+--------------------+
//  The object must be allocated at IRQL: | = PASSIVE_LEVEL    |  = PASSIVE_LEVEL   |
// ---------------------------------------+--------------------+--------------------+
//  The object must be freed at IRQL:     | = PASSIVE_LEVEL    |  = PASSIVE_LEVEL   |
// ---------------------------------------+--------------------+--------------------+
//  Constructor & destructor run at:      | = PASSIVE_LEVEL    |  = PASSIVE_LEVEL   |
// ---------------------------------------+--------------------+--------------------+
//  Member functions default to:          | PAGED code segment | .text code segment |
// ---------------------------------------+--------------------+--------------------+
//  Compiler-generated code goes to:      | PAGED code segment | .text code segment |
// ---------------------------------------+--------------------+--------------------+
//  The memory is allocated from pool:    | paged or nonpaged  | paged or nonpaged  |
// ---------------------------------------+--------------------+--------------------+
//

PAGED void *operator new(size_t s, std::nothrow_t const &, ULONG tag);
PAGED void operator delete(void *p, ULONG tag);
PAGED void *operator new[](size_t s, std::nothrow_t const &, ULONG tag);
PAGED void operator delete[](void *p, ULONG tag);
PAGEDX void __cdecl operator delete[](void *p);
void __cdecl operator delete(void *p);

template <ULONG TAG, ULONG ARENA = PagedPool>
struct KRTL_CLASS KALLOCATION_TAG
{
    static const ULONG AllocationTag = TAG;
    static const ULONG AllocationArena = ARENA;
};

template <ULONG TAG, ULONG ARENA = NonPagedPoolNx>
struct KRTL_CLASS_DPC_ALLOC KALLOCATION_TAG_DPC_ALLOC
{
    static const ULONG AllocationTag = TAG;
    static const ULONG AllocationArena = ARENA;
};

template <ULONG TAG, ULONG ARENA = PagedPool>
struct KRTL_CLASS KALLOCATOR : public KALLOCATION_TAG<TAG, ARENA>
{
    // Scalar new & delete

    PAGED void *operator new(size_t cb, std::nothrow_t const &)
    {
        PAGED_CODE();
        #pragma warning( suppress : 4996 28751 )
        return ExAllocatePoolWithTag(static_cast<POOL_TYPE>(ARENA), cb, TAG);
    }

    PAGED void operator delete(void *p)
    {
        PAGED_CODE();

        if (p != nullptr)
        {
            ExFreePoolWithTag(p, TAG);
        }
    }

    // Scalar new with bonus bytes

    PAGED void *operator new(size_t cb, std::nothrow_t const &, size_t extraBytes)
    {
        PAGED_CODE();

        auto size = cb + extraBytes;

        // Overflow check
        if (size < cb)
            return nullptr;

        #pragma warning( suppress : 4996 28751 )
        return ExAllocatePoolWithTag(static_cast<POOL_TYPE>(ARENA), size, TAG);
    }

    // Array new & delete

    PAGED void *operator new[](size_t cb, std::nothrow_t const &)
    {
        PAGED_CODE();
        #pragma warning( suppress : 4996 28751 )
        return ExAllocatePoolWithTag(static_cast<POOL_TYPE>(ARENA), cb, TAG);
    }

    PAGED void operator delete[](void *p)
    {
        PAGED_CODE();

        if (p != nullptr)
        {
            ExFreePoolWithTag(p, TAG);
        }
    }

    // Placement new & delete

    PAGED void *operator new(size_t n, void * p)
    {
        PAGED_CODE();
        UNREFERENCED_PARAMETER((n));
        return p;
    }

    PAGED void operator delete(void *p1, void *p2)
    {
        PAGED_CODE();
        UNREFERENCED_PARAMETER((p1, p2));
    }
};

template <ULONG TAG, ULONG ARENA = NonPagedPoolNx>
struct KRTL_CLASS_DPC_ALLOC KALLOCATOR_NONPAGED : public KALLOCATION_TAG_DPC_ALLOC<TAG, ARENA>
{
    // Scalar new & delete

    NONPAGED void *operator new(size_t cb, std::nothrow_t const &)
    {
        #pragma warning( suppress : 4996 28751 )
        return ExAllocatePoolWithTag(static_cast<POOL_TYPE>(ARENA), cb, TAG);
    }

    NONPAGED void operator delete(void *p)
    {
        if (p != nullptr)
        {
            ExFreePoolWithTag(p, TAG);
        }
    }

    // Scalar new with bonus bytes

    NONPAGED void *operator new(size_t cb, std::nothrow_t const &, size_t extraBytes)
    {
        auto size = cb + extraBytes;

        // Overflow check
        if (size < cb)
            return nullptr;

        #pragma warning( suppress : 4996 28751 )
        return ExAllocatePoolWithTag(static_cast<POOL_TYPE>(ARENA), size, TAG);
    }

    // Array new & delete

    NONPAGED void *operator new[](size_t cb, std::nothrow_t const &)
    {
        #pragma warning( suppress : 4996 28751 )
        return ExAllocatePoolWithTag(static_cast<POOL_TYPE>(ARENA), cb, TAG);
    }

    NONPAGED void operator delete[](void *p)
    {
        if (p != nullptr)
        {
            ExFreePoolWithTag(p, TAG);
        }
    }

    // Placement new & delete

    NONPAGED void *operator new(size_t n, void * p)
    {
        UNREFERENCED_PARAMETER((n));
        return p;
    }

    NONPAGED void operator delete(void *p1, void *p2)
    {
        UNREFERENCED_PARAMETER((p1, p2));
    }
};

template <ULONG TAG>
struct KRTL_CLASS PAGED_OBJECT :
    public KALLOCATOR<TAG, PagedPool>,
    public NdisDebugBlock<TAG>
{

};

template <ULONG TAG>
struct KRTL_CLASS NONPAGED_OBJECT :
    public KALLOCATOR<TAG, NonPagedPoolNx>,
    public NdisDebugBlock<TAG>
{

};

