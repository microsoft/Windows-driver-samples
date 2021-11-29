/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    MemoryKm.cpp

Abstract:

    Memory allocation and delete definitions

Environment:

    Kernel-mode.

--*/

#include "Pch.h"

#ifdef _KERNEL_MODE

void
__cdecl
operator delete(
    void* memory
    ) noexcept
{
    ExFreePoolWithTag(memory, TAG_UCSI);
}

void
__cdecl
operator delete[](
    void* memory
    ) noexcept
{
	ExFreePoolWithTag(memory, TAG_UCSI);
}

void*
__cdecl
operator new (
    size_t size
    )
{
    return ExAllocatePool2(POOL_FLAG_NON_PAGED, size, TAG_UCSI);
}

void*
__cdecl
operator new[] (
    size_t size
    )
{
    return ExAllocatePool2(POOL_FLAG_NON_PAGED, size, TAG_UCSI);
}

#endif
