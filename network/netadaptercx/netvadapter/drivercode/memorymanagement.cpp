// Copyright (c) Microsoft Corporation. All rights reserved
//
// Global operator new / delete for the driver.
//
// The shared netvadapterlibrary routes its allocations through the global
// operator new (see memory.cpp in the library, which provides the nothrow
// variant that forwards to operator new(size_t)). The client driver is
// responsible for providing the backing implementation. We use WDF managed
// memory so that the same implementation works for both KMDF and UMDF, the
// same approach used by the WIFICX sample.

#include "pch.hpp"

#define NETV_POOL_TAG 'vteN'

struct NETV_MEMORY_HEADER
{
    size_t HeaderSize;
    WDFMEMORY WdfMemoryHandle;
};

static void* AllocateBuffer(size_t Size)
{
    const size_t totalSize = Size + sizeof(NETV_MEMORY_HEADER);

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    WDFMEMORY wdfMemory = WDF_NO_HANDLE;
    void* buffer = nullptr;

    if (!NT_SUCCESS(WdfMemoryCreate(&attributes, NonPagedPoolNx, NETV_POOL_TAG, totalSize, &wdfMemory, &buffer)))
    {
        return nullptr;
    }

    RtlZeroMemory(buffer, totalSize);

    auto header = static_cast<NETV_MEMORY_HEADER*>(buffer);
    header->HeaderSize = sizeof(NETV_MEMORY_HEADER);
    header->WdfMemoryHandle = wdfMemory;

    return reinterpret_cast<void*>(reinterpret_cast<ULONG_PTR>(buffer) + sizeof(NETV_MEMORY_HEADER));
}

static void FreeBuffer(void* Buffer)
{
    if (Buffer == nullptr)
    {
        return;
    }

    auto header = reinterpret_cast<NETV_MEMORY_HEADER*>(
        reinterpret_cast<ULONG_PTR>(Buffer) - sizeof(NETV_MEMORY_HEADER));

    NT_ASSERT(header->HeaderSize == sizeof(NETV_MEMORY_HEADER));

    WdfObjectDelete(header->WdfMemoryHandle);
}

void* __cdecl operator new(size_t Size)
{
    return AllocateBuffer(Size);
}

void* __cdecl operator new[](size_t Size)
{
    return AllocateBuffer(Size);
}

void __cdecl operator delete(void* Buffer) noexcept
{
    FreeBuffer(Buffer);
}

void __cdecl operator delete[](void* Buffer) noexcept
{
    FreeBuffer(Buffer);
}

void __cdecl operator delete(void* Buffer, size_t) noexcept
{
    FreeBuffer(Buffer);
}

void __cdecl operator delete[](void* Buffer, size_t) noexcept
{
    FreeBuffer(Buffer);
}
