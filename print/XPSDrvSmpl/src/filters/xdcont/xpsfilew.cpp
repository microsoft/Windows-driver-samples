/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsfilew.cpp

Abstract:

   Implementation of an XPS file writer. This implements ISequentialStream::Write
   and essentially wraps a buffer that recieves and stores the part information
   so that in can be later compressed and written out.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xpsfilew.h"

/*++

Routine Name:

    CXPSWriteFile::CXPSWriteFile

Routine Description:

    CXPSWriteFile class constructor

Arguments:

    None

Return Value:

    None

--*/
CXPSWriteFile::CXPSWriteFile() :
    CUnknown<ISequentialStream>(IID_ISequentialStream),
    m_cbWritten(0)
{
}

/*++

Routine Name:

    CXPSWriteFile::CXPSWriteFile

Routine Description:

    CXPSWriteFile class constructor

Arguments:

    szFileName - The name of the file to write to

Return Value:

    None

--*/
CXPSWriteFile::CXPSWriteFile(
    PCSTR szFileName
    ) :
    CUnknown<ISequentialStream>(IID_ISequentialStream),
    m_cbWritten(0),
    m_cstrFileName(szFileName)
{
}

/*++

Routine Name:

    CXPSWriteFile::~CXPSWriteFile

Routine Description:

    CXPSWriteFile class destructor

Arguments:

    None

Return Value:

    None

--*/
CXPSWriteFile::~CXPSWriteFile()
{
}

//
// ISequentialStream members
//
/*++

Routine Name:

    CXPSWriteFile::Read

Routine Description:

    This is the ISequentialStream read method - this is not implemented in
    the file writter

Arguments:

    Unused

Return Value:

    HRESULT
    E_NOTIMPL - This method is not implemented

--*/
HRESULT STDMETHODCALLTYPE
CXPSWriteFile::Read(
    _Out_writes_bytes_to_(cb, *pcbRead) void*,
    _In_ ULONG cb,
    _Out_opt_ ULONG* pcbRead
    )
{
    UNREFERENCED_PARAMETER(cb);
    UNREFERENCED_PARAMETER(pcbRead);
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXPSWriteFile::Write

Routine Description:

    This routine implements the ISequentialStream wrte interface allowing
    clients to write file data.

Arguments:

    pData      - Pointer to the source data
    cbData     - The size of the data buffer
    pcbWritten - Pointer to a ULONG that recieves the number of bytes written

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CXPSWriteFile::Write(
    _In_reads_bytes_(cbData) CONST void* pData,
    _In_                ULONG       cbData,
    _Out_               ULONG*      pcbWritten
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcbWritten, E_POINTER)))
    {
        *pcbWritten = 0;

        PBYTE pDest = NULL;
        if (SUCCEEDED(hr = m_workFile.GetBuffer(m_cbWritten + cbData, reinterpret_cast<PVOID*>(&pDest))))
        {
            pDest += m_cbWritten;

            CopyMemory(pDest, pData, cbData);

            m_cbWritten += cbData;
            *pcbWritten = cbData;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSWriteFile::GetBuffer

Routine Description:

    This routine retrieves the buffer containing the file data written by
    a client

Arguments:

    ppv - Pointer to a VOID pointer that recieves the buffer
    pcb - Pointer to a ULONG that recieves the size of the buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSWriteFile::GetBuffer(
    _Outptr_result_bytebuffer_(*pcb) PVOID* ppv,
    _Out_                    ULONG* pcb
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppv, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcb, E_POINTER)))
    {
        *ppv = NULL;
        *pcb = 0;

        if (SUCCEEDED(hr = m_workFile.GetBuffer(m_cbWritten, ppv)))
        {
            *pcb = static_cast<ULONG>(m_cbWritten);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSWriteFile::GetFileName

Routine Description:

    This routine retrieves the file name of the currently opened file

Arguments:

    pszFileName - Pointer to a string that recieves the filename

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSWriteFile::GetFileName(
    _Outptr_ PSTR* pszFileName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pszFileName, E_POINTER)))
    {
        try
        {
            if (m_cstrFileName.GetLength() > 0)
            {
                *pszFileName = m_cstrFileName.GetBuffer();
            }
            else
            {
                hr = E_PENDING;
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
