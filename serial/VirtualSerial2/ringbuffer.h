/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Ringbuffer.h

--*/

#pragma once

typedef struct _RING_BUFFER
{
    //
    // The size in bytes of the ring buffer.
    //
    size_t          Size;

    //
    // A pointer to the base of the ring buffer.
    //
    BYTE*           Base;

    //
    // A pointer to the byte beyond the end of the ring buffer.  Used for
    // quick comparisons when determining if we need to wrap.
    //
    BYTE*           End;

    //
    // A pointer to the current read point in the ring buffer.
    //
    // Updates to this are not protected by any lock. This is different from
    // the write pointer, which is protected by the "pending read pointer"
    // lock. The reason for this difference is that in this driver, we do not
    // keep write requests pending. If there is not enough space to write all
    // the data that was requested, we write as much as we can and drop the
    // rest (lossy data transfer).
    //
    // If we had multiple threads modifying this pointer, then that would
    // provide yet another reason for protecting updates to the pointer using a
    // lock. However, in this driver, at any given time we have only one thread
    // that modifies this pointer (the thread that runs the read callback).
    // This is true because we use a sequential queue for read requests. If we
    // were to change our read queue to be a parallel queue, this would no
    // longer be true.
    //
    //
    BYTE*           Head;

    //
    // A pointer to the current write point in the ring buffer.
    //
    // Updates to this pointer are protected by the "pending read pointer
    // lock", because we do not want a consumer thread to mark a read request
    // as pending while we are in the process of writing data to the buffer.
    // The reason is that the write that we are currently performing might
    // actually supply enough data to satisfy the read request, in which case
    // it should not be marked pending at all.
    // If the read request were to be marked pending in the situation described
    // above, then we would need some trigger to later retrieve the request and
    // complete it. In our driver, arrival of data is the only event that can
    // trigger this. So if no more data arrives, the request will remain
    // pending forever, even though there is enough data in the buffer to
    // complete it. Hence we do not keep a read request pending in situations
    // where the read buffer contains enough data to satisfy it.
    //
    // If we had multiple threads modifying this pointer, then that would
    // provide yet another reason for protecting updates to the pointer using a
    // lock. However, in this driver, at any given time we have only one thread
    // that modifies this pointer (the thread that runs the write callback).
    // This is true because we use a sequential queue for write requests. If we
    // were to change our write queue to be a parallel queue, this would no
    // longer be true.
    //
    BYTE*           Tail;

} RING_BUFFER, *PRING_BUFFER;


VOID
RingBufferInitialize(
    _In_  PRING_BUFFER      Self,
    _In_reads_bytes_(BufferSize)
          BYTE*             Buffer,
    _In_  size_t            BufferSize
    );

NTSTATUS
RingBufferWrite(
    _In_  PRING_BUFFER      Self,
    _In_reads_bytes_(DataSize)
          BYTE*             Data,
    _In_  size_t            DataSize
    );

NTSTATUS
RingBufferRead(
    _In_  PRING_BUFFER      Self,
    _Out_writes_bytes_to_(DataSize, *BytesCopied)
          BYTE*             Data,
    _In_  size_t            DataSize,
    _Out_ size_t            *BytesCopied
    );

VOID
RingBufferGetAvailableSpace(
    _In_  PRING_BUFFER      Self,
    _Out_ size_t            *AvailableSpace
    );

VOID
RingBufferGetAvailableData(
    _In_  PRING_BUFFER      Self,
    _Out_ size_t            *AvailableData
    );
