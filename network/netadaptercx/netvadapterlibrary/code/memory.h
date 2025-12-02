// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once
#include "adapter.h"
#include <KSpinLock.h>

static const size_t PREALLOCATED_BUFFERS_COUNT = 128;

struct MemoryBuffer
{
    NET_MEMORY_ID
        MemoryId;
    
    BYTE
        VirtualAddress[MAX_RX_BUFFER_SIZE];
};

class Memory
{
public:

    NTSTATUS
    Initialize(
        NETMEMORYCOLLECTION MemoryCollection,
        size_t BufferSize
    );

    MemoryBuffer*
    PopAvailableBuffer(
        void
    );

    void
    ReturnBuffer(
        MemoryBuffer* Buffer
    );

    void
    ReleaseAllMappings(
        void
    );

private:
    NETMEMORYCOLLECTION
        m_memoryCollection;

    MemoryBuffer
        m_buffers[PREALLOCATED_BUFFERS_COUNT];

    MemoryBuffer*
        m_buffersReadyToUse[PREALLOCATED_BUFFERS_COUNT];

    size_t
        m_lastBufferToUse;

    KSpinLock
        m_spinLock{};
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Memory, GetMemoryFromHandle);
