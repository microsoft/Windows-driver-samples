/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   workbuff.cpp

Abstract:

   CWorkingBuffer class implementation. This class provides a means of working
   with a local buffer by defining a simple interface that allows the retrieval
   of a buffer of an appropriate size, handling reallocation as necessary.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "workbuff.h"

/*++

Routine Name:

    CWorkingBuffer::CWorkingBuffer

Routine Description:

    CWorkingBuffer class constructor

Arguments:

    None

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CWorkingBuffer::CWorkingBuffer() :
    m_pBuffer(NULL),
    m_cbSize(CB_COPY_BUFFER)
{
    m_pBuffer = HeapAlloc(GetProcessHeap(), 0, m_cbSize);

    if (m_pBuffer == NULL)
    {
        m_cbSize = 0;
        throw CXDException(E_OUTOFMEMORY);
    }
}

/*++

Routine Name:

    CWorkingBuffer::~CWorkingBuffer

Routine Description:

    CWorkingBuffer class destructor

Arguments:

    None

Return Value:

    None

--*/
CWorkingBuffer::~CWorkingBuffer()
{
    if (m_pBuffer != NULL)
    {
        HeapFree(GetProcessHeap(), 0, m_pBuffer);
        m_pBuffer = NULL;
        m_cbSize = 0;
    }
}

/*++

Routine Name:

    CWorkingBuffer::GetBuffer

Routine Description:

    This routine returns a pointer to a buffer of at least the requested size. If the internal
    buffer is not large enough it will be resized accordingly.

Arguments:

    cbSize   - Requested size of the buffer
    ppBuffer - Pointer to a VOID pointer that recieves the buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWorkingBuffer::GetBuffer(
    _In_                       CONST ULONGLONG cbSize,
    _Outptr_result_bytebuffer_(cbSize) PVOID*          ppBuffer
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppBuffer, E_POINTER)))
    {
        *ppBuffer = NULL;

        if (cbSize == 0)
        {
            hr = E_INVALIDARG;
        }

        if (m_cbSize == 0)
        {
            //
            // We have no existing buffer - we should have one
            //
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (m_cbSize >= cbSize)
        {
            *ppBuffer = m_pBuffer;
        }
        else
        {
            //
            // Double the buffer size till it is greater than the requested size
            //
            SIZE_T cbNewSize = m_cbSize;
            while (cbNewSize < cbSize)
            {
                cbNewSize *= 2;
            };

            _Analysis_assume_(cbNewSize >= 1);

            //
            // Reallocate the buffer
            //
            PVOID pNewBuffer = HeapReAlloc(GetProcessHeap(), 0, m_pBuffer, cbNewSize);

            if (pNewBuffer != NULL)
            {
                m_cbSize = cbNewSize;
                m_pBuffer = pNewBuffer;
                *ppBuffer = m_pBuffer;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
};

/*++

Routine Name:

    CWorkingBuffer::GetBufferAt

Routine Description:

    This routine returns a pointer into the internal buffer at the requested offset. This will
    not grow the size of the buffer.

Arguments:

    cbOffset - Offset into the internal buffer
    cbSize   - Size of the requested buffer
    ppBuffer - Pointer to a VOID pointer that recieves the buffer

Return Value:

    HRESULT
    S_OK                                          - On success
    HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) - If the internal buffer is not large enough
    E_*                                           - On error

--*/
HRESULT
CWorkingBuffer::GetBufferAt(
    _In_                       ULONG           cbOffset,
    _In_                       CONST ULONGLONG cbSize,
    _Outptr_result_bytebuffer_(cbSize) PVOID*          ppBuffer
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppBuffer, E_POINTER)))
    {
        *ppBuffer = NULL;

        if (cbSize + cbOffset < m_cbSize)
        {
            *ppBuffer = reinterpret_cast<PBYTE>(m_pBuffer) + cbOffset;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

