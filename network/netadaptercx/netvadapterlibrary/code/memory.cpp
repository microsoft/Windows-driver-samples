// Copyright (C) Microsoft Corporation. All rights reserved.
#include "pch.hpp"
#include "memory.h"
#include <netadaptercx.h>
#include "trace.h"
#include "memory.tmh"

#if ((NETADAPTER_VERSION_MAJOR == 2) && (NETADAPTER_VERSION_MINOR >= 6))

NTSTATUS
Memory::Initialize(
    NETMEMORYCOLLECTION MemoryCollection,
    size_t BufferSize
)
{
    m_memoryCollection = MemoryCollection;
    
    for (size_t i = 0; i < PREALLOCATED_BUFFERS_COUNT; i++)
    {
        NET_MEMORY_CONFIG memoryConfig{};
        NET_MEMORY_CONFIG_INIT(
            &memoryConfig,
            m_buffers[i].VirtualAddress,
            BufferSize);

        RETURN_IF_NOT_STATUS_SUCCESS(
            NetMemoryCreate(
                MemoryCollection,
                &memoryConfig,
                &m_buffers[i].MemoryId
            ));
         
        m_buffersReadyToUse[i] = &m_buffers[i];
    }

    m_lastBufferToUse = PREALLOCATED_BUFFERS_COUNT;
    return STATUS_SUCCESS;
}

void
Memory::ReleaseAllMappings(
    void
)
{
    for (size_t i = 0; i < PREALLOCATED_BUFFERS_COUNT; i++)
    {
        NetMemoryDestroy(m_memoryCollection, m_buffers[i].MemoryId);
    }

    m_lastBufferToUse = 0;
    RtlZeroMemory(m_buffersReadyToUse, sizeof(m_buffersReadyToUse));
}

MemoryBuffer*
Memory::PopAvailableBuffer(
    void
)
{
    KAcquireSpinLock lock{ m_spinLock };

    if (m_lastBufferToUse == 0)
    {
        return nullptr;
    }

    return m_buffersReadyToUse[--m_lastBufferToUse];
}

void
Memory::ReturnBuffer(
    MemoryBuffer* Buffer
)
{
    KAcquireSpinLock lock{ m_spinLock };

    NT_FRE_ASSERT(m_lastBufferToUse < PREALLOCATED_BUFFERS_COUNT);

    m_buffersReadyToUse[m_lastBufferToUse++] = Buffer;
}
#endif //NETCX 2.6 only

// for wil::make_unique_nothrow
void*
operator new(size_t s, std::nothrow_t const&)
{
    return operator new(s);
}