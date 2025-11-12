// Copyright (C) Microsoft Corporation. All rights reserved.
#include "precomp.h"

#ifndef _KERNEL_MODE
#include <intrin.h> // for _ReturnAddress for user mode
#endif // UM

typedef struct _PLACEMENT_NEW_ALLOCATION_CONTEXT
{
    size_t cbMaxSize;
    _Field_size_bytes_(cbMaxSize) void* pbBuffer;
} PLACEMENT_NEW_ALLOCATION_CONTEXT, * PPLACEMENT_NEW_ALLOCATION_CONTEXT;
typedef const PLACEMENT_NEW_ALLOCATION_CONTEXT* PCPLACEMENT_NEW_ALLOCATION_CONTEXT;

// for FreeWdfMemoryBuffer to correct get
// the handle to free the memory
struct WIFI_IHV_MEMORY_HEADER
{
    size_t HeaderSize;
    WDFMEMORY WdfMemoryHandle;
};

// for tracking memory leaks
struct WIFI_IHV_MEMORY_CONTEXT
{
    size_t ContextSize;
    void* pvCaller;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WIFI_IHV_MEMORY_CONTEXT, GetWificxIhvMemoryContextFromHandle);


void* AllocateWdfMemoryBuffer(size_t Size, _In_ void* CallerForMemoryLeakTracking)
{
    size_t totalSize = 0;
    if (!NT_SUCCESS(Wifi::SizeTAddSafe(Size, sizeof(WIFI_IHV_MEMORY_HEADER), &totalSize)))
    {
        NT_ASSERT(FALSE);
        WFCError("Failed to calculate total size");
        return nullptr;
    }

    // allocate the context memory to store the WIFI_IHV_MEMORY_CONTEXT
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WIFI_IHV_MEMORY_CONTEXT);
    WDFMEMORY wdfMemoryBufferHandle = WDF_NO_HANDLE;
    void* pWdfMemoryBuffer = nullptr;

    if (!NT_SUCCESS(WdfMemoryCreate(&attributes, NonPagedPoolNx, WIFI_DRIVER_DEFAULT_POOL_TAG, totalSize, &wdfMemoryBufferHandle, &pWdfMemoryBuffer)))
    {
        WFCError("Failed to allocate memory buffer");
        return nullptr;
    }

    RtlZeroMemory(pWdfMemoryBuffer, totalSize);

    // store the wdf memory handle in the header for FreeWdfMemoryBuffer to find the handle
    WIFI_IHV_MEMORY_HEADER* pMemoryHeader = static_cast<WIFI_IHV_MEMORY_HEADER*>(pWdfMemoryBuffer);
    pMemoryHeader->HeaderSize = sizeof(WIFI_IHV_MEMORY_HEADER);
    pMemoryHeader->WdfMemoryHandle = wdfMemoryBufferHandle;

    // store the caller and caller's caller for tracking memory leaks
    auto memoryContext = GetWificxIhvMemoryContextFromHandle(wdfMemoryBufferHandle);
    RtlZeroMemory(memoryContext, sizeof(WIFI_IHV_MEMORY_CONTEXT));
    memoryContext->ContextSize = sizeof(WIFI_IHV_MEMORY_CONTEXT);
    memoryContext->pvCaller = CallerForMemoryLeakTracking;

    // hide the wdf memory header to the caller.
    const ULONG_PTR wdfMemoryWithHeaderBuffer = reinterpret_cast<ULONG_PTR>(pWdfMemoryBuffer);
    ULONG_PTR wdfMemoryPayloadOnlyBuffer{ 0 };
    if (!NT_SUCCESS(Wifi::ULongPtrAddSafe(wdfMemoryWithHeaderBuffer, sizeof(WIFI_IHV_MEMORY_HEADER), &wdfMemoryPayloadOnlyBuffer)))
    {
        NT_ASSERT(FALSE);
        WFCError("Failed to calculate payload buffer address");
        return nullptr;
    }

    return reinterpret_cast<void*>(wdfMemoryPayloadOnlyBuffer);
}

void FreeWdfMemoryBuffer(_In_opt_ void* pBuffer)
{
    if (pBuffer == nullptr) // existing wdi code not checking for nullptr before delete, so we have to leave it as is
    {
        return;
    }
    const ULONG_PTR wdfMemoryPayloadOnlyBuffer = reinterpret_cast<ULONG_PTR>(pBuffer);
    ULONG_PTR wdfMemoryWithHeaderBuffer{ 0 };
    if (!NT_SUCCESS(Wifi::ULongPtrSubSafe(wdfMemoryPayloadOnlyBuffer, sizeof(WIFI_IHV_MEMORY_HEADER), &wdfMemoryWithHeaderBuffer)))
    {
        NT_ASSERT(FALSE);
        WFCError("Failed to calculate header buffer address");
        return;
    }

    const auto wificxMemoryHeader = static_cast<WIFI_IHV_MEMORY_HEADER*>(reinterpret_cast<void*>(wdfMemoryWithHeaderBuffer));

    // This is memory corruption detection logic,
    // keep it, don't remove it.
    NT_ASSERT(wificxMemoryHeader->HeaderSize == sizeof(WIFI_IHV_MEMORY_HEADER));

    WdfObjectDelete(wificxMemoryHeader->WdfMemoryHandle);
}

_Ret_writes_bytes_maybenull_(_Size) void* PlacementNewHelper(size_t _Size, PCPLACEMENT_NEW_ALLOCATION_CONTEXT AllocationContext)
{
    if (_Size <= AllocationContext->cbMaxSize)
    {
        RtlZeroMemory(AllocationContext->pbBuffer, _Size);
        return AllocationContext->pbBuffer;
    }
    WFCError(
        "Placement operator new called with insufficient buffer space (desired: %Iu, availible: %Iu)", _Size, AllocationContext->cbMaxSize);
    return nullptr;
}

void* __cdecl operator new(size_t Size)
{
    return AllocateWdfMemoryBuffer(Size, _ReturnAddress());
}

__forceinline void* __cdecl operator new(size_t _Size, ULONG_PTR AllocationContext) throw()
{
    if (AllocationContext != 0)
    {
        return PlacementNewHelper(_Size, (PPLACEMENT_NEW_ALLOCATION_CONTEXT)AllocationContext);
    }
    return AllocateWdfMemoryBuffer(_Size, _ReturnAddress());
}

void __cdecl operator delete(void* pData)
{
    FreeWdfMemoryBuffer(pData);
}

void __cdecl operator delete[](void* pData)
{
    FreeWdfMemoryBuffer(pData);
}

void __cdecl operator delete(void* pData, ULONG_PTR)
{
    FreeWdfMemoryBuffer(pData);
}

void __cdecl operator delete[](void* pData, ULONG_PTR)
{
    FreeWdfMemoryBuffer(pData);
}
