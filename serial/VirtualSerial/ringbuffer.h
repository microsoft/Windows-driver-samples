#pragma once

#include "internal.h"

class CRingBuffer
{
private:
                                                           
    //
    // Private local variables.
    //

    //
    // The size in bytes of the ring buffer.
    //
    SIZE_T m_Size;

    //
    // A pointer to the base of the ring buffer.
    //
    PBYTE m_Base;

    //
    // A pointer to the byte beyond the end of the ring buffer.  Used for
    // quick comparisons when determining if we need to wrap.
    //
    PBYTE m_End;

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
    PBYTE m_Head;

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
    PBYTE m_Tail;

private:

    //
    // Private Internal Methods.
    //
    void
    GetAvailableSpace(
        _Out_ SIZE_T *AvailableSpace
        )
    {
        WUDF_TEST_DRIVER_ASSERT(AvailableSpace);

        PBYTE headSnapshot = NULL;
        PBYTE tailSnapshot = NULL;
        PBYTE tailPlusOne = NULL;

        //
        // Take a snapshot of the head and tail pointers. We will compute the
        // available space based on this snapshot. This is safe to do in a 
        // single-producer, single-consumer model, because -
        //     * A producer will call GetAvailableSpace() to determine whether
        //       there is enough space to write the data it is trying to write.
        //       The only other thread that could modify the amount of space 
        //       available is the consumer thread, which can only increase the
        //       amount of space available. Hence it is safe for the producer
        //       to write based on this snapshot.
        //     * A consumer thread will call GetAvailableSpace() to determine
        //       whether there is enough data in the buffer for it to read. 
        //       (Available data = Buffer size - Available space). The only 
        //       other thread that could modify the amount of space available
        //       is the producer thread, which can only decrease the amount of
        //       space available (thereby increasing the amount of data 
        //       available. Hence it is safe for the consumer to read based on 
        //       this snapshot.
        //
        headSnapshot = m_Head;
        tailSnapshot = m_Tail;

        //
        // In order to distinguish between a full buffer and an empty buffer,
        // we always leave the last byte of the buffer unused. So, an empty
        // buffer is denoted by -
        //      tail == head 
        // ... and a full buffer is denoted by -
        //      (tail+1) == head
        //
        tailPlusOne = ((tailSnapshot+1) == m_End) ? m_Base : (tailSnapshot+1);

        if (tailPlusOne == headSnapshot)
        {
            //
            // Buffer full
            //
            *AvailableSpace = 0;
        }
        else if (tailSnapshot == headSnapshot)
        {
            //
            // Buffer empty
            // The -1 in the computation below is to account for the fact that
            // we always leave the last byte of the ring buffer unused in order
            // to distinguish between an empty buffer and a full buffer.
            //
            *AvailableSpace = m_Size - 1;
        }
        else
        {
            if (tailSnapshot > headSnapshot)
            {
                //
                // Data has not wrapped around the end of the buffer
                // The -1 in the computation below is to account for the fact 
                // that we always leave the last byte of the ring buffer unused
                // in order to distinguish between an empty buffer and a full
                // buffer.
                //
                *AvailableSpace = m_Size - (tailSnapshot - headSnapshot) - 1;
            }
            else
            {
                //
                // Data has wrapped around the end of the buffer
                // The -1 in the computation below is to account for the fact 
                // that we always leave the last byte of the ring buffer unused
                // in order to distinguish between an empty buffer and a full
                // buffer.
                //
                *AvailableSpace = (headSnapshot - tailSnapshot) - 1;
            }
        }

        return;
    }

public:

    //
    // Public Internal methods.
    //

    CRingBuffer(
        VOID
        );

    ~CRingBuffer(
        VOID
        );

    HRESULT
    Initialize(
        _In_ SIZE_T BufferSize
        );

    HRESULT
    Write(
        _In_reads_bytes_(DataSize) PBYTE Data,
        _In_ SIZE_T DataSize
        );
    
    HRESULT
    Read(
        _Out_writes_bytes_to_(DataSize, *BytesCopied) PBYTE Data,
        _In_ SIZE_T DataSize,
        _Out_ SIZE_T *BytesCopied
        );

    void
    GetAvailableData(
        _Out_ SIZE_T *AvailableData
        )
    {
        SIZE_T availableSpace;

        WUDF_TEST_DRIVER_ASSERT(AvailableData);

        GetAvailableSpace(&availableSpace);

        //
        // The -1 in the arithmetic below accounts for the fact that we always
        // keep 1 byte of the ring buffer unused in order to distinguish 
        // between a full buffer and an empty buffer.
        //
        *AvailableData = m_Size - availableSpace - 1;

        return;
    }
};
