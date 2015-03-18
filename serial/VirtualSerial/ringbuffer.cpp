#include "internal.h"

CRingBuffer::CRingBuffer(
    VOID
    ) : m_Size(0),
        m_Base(NULL),
        m_End(NULL),
        m_Head(NULL),
        m_Tail(NULL)
{
    return;
}

CRingBuffer::~CRingBuffer(
    VOID
    )
{
    if (m_Base)
    {
        delete[] m_Base;
    }
}

HRESULT
CRingBuffer::Initialize(
    _In_ SIZE_T BufferSize
    )
{
    HRESULT hr = S_OK;
    PBYTE buffer = NULL;

    if (0 == BufferSize)
    {
        hr = E_INVALIDARG;
    }

    //
    // Allocate the buffer.
    //
    if (SUCCEEDED(hr))
    {
        buffer = new BYTE[BufferSize];
        if (buffer == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Initialize the ring buffer pointers.
    //
    if (SUCCEEDED(hr))
    {
        m_Size = BufferSize;
        m_Base = buffer;
        m_End = buffer + BufferSize;
        m_Head = buffer;
        m_Tail = buffer;
    }

    if (FAILED(hr))
    {
        if (buffer)
        {
            delete[] buffer;
            m_Base = NULL;
        }
    }

    return hr;
}

HRESULT
CRingBuffer::Write(
    _In_reads_bytes_(DataSize) PBYTE Data,
    _In_ SIZE_T DataSize
    )
{
    SIZE_T availableSpace;
    SIZE_T bytesToCopy;
    SIZE_T spaceFromCurrToEnd;

    WUDF_TEST_DRIVER_ASSERT(Data && (0 != DataSize));

    if (m_Tail >= m_End)
    {
        return E_UNEXPECTED;
    }

    //
    // Get the amount of space available in the buffer
    //
    GetAvailableSpace(&availableSpace);

    //
    // If there is not enough space to fit in all the data passed in by the 
    // caller then copy as much as possible and throw away the rest
    //
    if (availableSpace < DataSize)
    {
        bytesToCopy = availableSpace;
    }
    else
    {
        bytesToCopy = DataSize;
    }

    if (bytesToCopy)
    {
        //
        // The buffer has some space at least
        //
        if ((m_Tail+bytesToCopy) > m_End)
        {
            //
            // The data being written will wrap around the end of the buffer. 
            // So the copy has to be done in two steps -
            // * X bytes from current position to end of the buffer
            // * the remaining (bytesToCopy - X) from the start of the buffer
            //

            //
            // The first step of the copy ...
            //
            spaceFromCurrToEnd = m_End - m_Tail;
            CopyMemory(m_Tail, Data, spaceFromCurrToEnd);
            Data += spaceFromCurrToEnd;
            bytesToCopy -= spaceFromCurrToEnd;

            //
            // The second step of the copy ...
            //
            CopyMemory(m_Base, Data, bytesToCopy);

            //
            // Advance the tail pointer 
            //
            m_Tail = m_Base + bytesToCopy;
        }
        else
        {
            //
            // Data does NOT wrap around the end of the buffer. Just copy it 
            // over in a single step
            //
            CopyMemory(m_Tail, Data, bytesToCopy);

            //
            // Advance the tail pointer
            //
            m_Tail += bytesToCopy;
            if (m_Tail == m_End)
            {
                //
                // We have exactly reached the end of the buffer. The next 
                // write should wrap around and start from the beginning.
                //
                m_Tail = m_Base;
            }
        }

        WUDF_TEST_DRIVER_ASSERT(m_Tail < m_End);
    }

    return S_OK;
}

HRESULT
CRingBuffer::Read(
    _Out_writes_bytes_to_(DataSize, *BytesCopied) PBYTE Data,
    _In_ SIZE_T DataSize,
    _Out_ SIZE_T *BytesCopied
    )
{
    SIZE_T availableData;
    SIZE_T dataFromCurrToEnd;

    WUDF_TEST_DRIVER_ASSERT(Data && (DataSize != 0));

    if (m_Head >= m_End)
    {
        return E_UNEXPECTED;
    }

    //
    // Get the amount of data available in the buffer
    //
    GetAvailableData(&availableData);

    if (availableData == 0)
    {
        *BytesCopied = 0;
        return S_OK;
    }
    
    if (DataSize > availableData)
    {
        DataSize = availableData;
    }
    
    *BytesCopied = DataSize;

    if ((m_Head + DataSize) > m_End)
    {
        //
        // The data requested by the caller is wrapped around the end of the
        // buffer. So we'll do the copy in two steps -
        //    * Copy X bytes from the current position to the end buffer into 
        //      the caller's buffer
        //    * Copy (DataSize - X) bytes from the beginning to the buffer into
        //      the caller's buffer
        //

        //
        // The first step of the copy ...
        //
        dataFromCurrToEnd = m_End - m_Head;
        CopyMemory(Data, m_Head, dataFromCurrToEnd);
        Data += dataFromCurrToEnd;
        DataSize -= dataFromCurrToEnd;

        //
        // The second step of the copy ...
        //
        CopyMemory(Data, m_Base, DataSize);

        //
        // Advance the head pointer
        //
        m_Head = m_Base + DataSize;
    }
    else
    {
        //
        // The data in the buffer is NOT wrapped around the end of the buffer.
        // Simply copy the data over to the caller's buffer in a single step.
        //
        CopyMemory(Data, m_Head, DataSize);

        //
        // Advance the head pointer
        //
        m_Head += DataSize;
        if (m_Head == m_End)
        {
            //
            // We have exactly reached the end of the buffer. The next 
            // read should wrap around and start from the beginning.
            //
            m_Head = m_Base;
        }
    }

    WUDF_TEST_DRIVER_ASSERT(m_Head < m_End);

    return S_OK;
}

