/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   widetoutf8.cpp

Abstract:

   The CWideToUTF8 class converts a Unicode character string into the UTF-8 code page format.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "widetoutf8.h"

/*++

Routine Name:

    CWideToUTF8::CWideToUTF8

Routine Description:

    CWideToUTF8 class constructor

Arguments:

    pcstrWide - Unicode string to be converted to UTF8.

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CWideToUTF8::CWideToUTF8(
    CStringXDW* pcstrWide
    ) :
    m_pcstrWide(pcstrWide),
    m_pMultiByte(NULL)
{
    HRESULT hr = CHECK_POINTER(m_pcstrWide, E_POINTER);

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CWideToUTF8::~CWideToUTF8

Routine Description:

    CWideToUTF8 class destructor

Arguments:

    None

Return Value:

    None

--*/
CWideToUTF8::~CWideToUTF8()
{
    FreeBuffer();
}

/*++

Routine Name:

    CWideToUTF8::GetBuffer

Routine Description:

    This method retrieves a pointer to the UTF8 converted character buffer for the CWideToUTF8 object.

Arguments:

    ppBuffer - pointer to a pointer to the buffer.
    pcbBuffer - size of the buffer that was returned.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWideToUTF8::GetBuffer(
    _Outptr_ PVOID* ppBuffer,
    _Out_       ULONG* pcbBuffer
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppBuffer, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcbBuffer, E_POINTER)))
    {
        FreeBuffer();

        try
        {
            INT cbMultiByte = WideCharToMultiByte(CP_UTF8,
                                                  0,
                                                  m_pcstrWide->GetBuffer(),
                                                  m_pcstrWide->GetLength(),
                                                  NULL,
                                                  0,
                                                  NULL,
                                                  NULL);

            if (cbMultiByte > 0)
            {
                m_pMultiByte = new(std::nothrow) CHAR[cbMultiByte];

                if (SUCCEEDED(hr = CHECK_POINTER(m_pMultiByte, E_OUTOFMEMORY)))
                {
                    if (WideCharToMultiByte(CP_UTF8,
                                                  0,
                                                  m_pcstrWide->GetBuffer(),
                                                  m_pcstrWide->GetLength(),
                                                  m_pMultiByte,
                                                  cbMultiByte,
                                                  NULL,
                                                  NULL) > 0)
                    {
                        *ppBuffer = m_pMultiByte;
                        *pcbBuffer = cbMultiByte;
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWideToUTF8::FreeBuffer

Routine Description:

    Releases the memory that was allocated for the string buffer during a call to CWideToUTF8::GetBuffer.

Arguments:

    None

Return Value:

    None

--*/
VOID
CWideToUTF8::FreeBuffer()
{
    if (m_pMultiByte != NULL)
    {
        delete[] m_pMultiByte;
        m_pMultiByte = NULL;
    }
}

