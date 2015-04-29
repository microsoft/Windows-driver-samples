/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsfiler.cpp

Abstract:

   Implementation of the XPS file reader class. This class implements
   ISequentialStream::Read by using the IPKFile interface to supply the
   client with decompressed data as requested. This allows the file to
   be passed directly to a SAX or DOM handler for parsing.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xpsfiler.h"

/*++

Routine Name:

    CXPSReadFile::CXPSReadFile

Routine Description:

    CXPSReadFile class constructor

Arguments:

    None

Return Value:

    None

--*/
CXPSReadFile::CXPSReadFile() :
    CUnknown<ISequentialStream>(IID_ISequentialStream),
    m_cbSent(0),
    m_cbExtracted(0),
    m_bExtractedAll(FALSE)
{
}

/*++

Routine Name:

    CXPSReadFile::~CXPSReadFile

Routine Description:

    CXPSReadFile class denstructor

Arguments:

    None

Return Value:

    None

--*/
CXPSReadFile::~CXPSReadFile()
{
    Close();
}

/*++

Routine Name:

    CXPSReadFile::Read

Routine Description:

    This routine implements the ISequentialStream read interface allowing
    clients to access file data without having to worry about part
    interleaving or decompression.

Arguments:

    pv      - Pointer to the buffer to recieve the file data
    cb      - The size of the data buffer
    pcbRead - Pointer to a ULONG that recieves the number of bytes read

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT STDMETHODCALLTYPE
CXPSReadFile::Read(
    _Out_writes_bytes_(cb) void*  pv,
    _In_             ULONG  cb,
    _Out_            ULONG* pcbRead
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pv, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcbRead, E_POINTER)))
    {
        *pcbRead = 0;

        if (cb > 0)
        {
            //
            // Add data to our working buffer till we have enough to copy or
            // we ran out of data
            //
            while (m_cbExtracted - m_cbSent < cb &&
                   !m_bExtractedAll &&
                   SUCCEEDED(hr))
            {
                hr = DecompressNextFile();
            }

            //
            // Copy the data into the copy buffer
            //
            if (SUCCEEDED(hr))
            {
                hr = CopyBuffer(pv, cb, pcbRead);
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSReadFile::Write

Routine Description:

    This routine is the ISequentialStream write implementation. It is not
    implemented

Arguments:

    Unused

Return Value:

    HRESULT
    E_NOTIMPL - This method is not implemented

--*/
HRESULT STDMETHODCALLTYPE
CXPSReadFile::Write(
    _In_reads_bytes_(cb) CONST void*,
    _In_ ULONG cb,
    _Out_opt_ ULONG*
    )
{
    UNREFERENCED_PARAMETER(cb);
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXPSReadFile::Open

Routine Description:

    This routine intialises a file ready to be read

Arguments:

    szFileName - The name of the part to be intialised

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSReadFile::Open(
    _In_ PCSTR szFileName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szFileName, E_POINTER)))
    {
        try
        {
            if (m_partStack.empty())
            {
                m_cstrFileName = szFileName;
            }
            else
            {
                RIP("The file must be closed before another is opened\n");

                hr = E_FAIL;
            }

        }
        catch (CXDException& e)
        {
            hr = e;
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSReadFile::Close

Routine Description:

    This routine closes the currently open file

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSReadFile::Close(
    VOID
    )
{
    HRESULT hr = S_OK;

    m_partStack.clear();
    m_cstrFileName.Empty();
    m_cbSent = 0;
    m_cbExtracted = 0;
    m_bExtractedAll = 0;

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSReadFile::GetBuffer

Routine Description:

    This routine retrieves the current populated buffer and the count
    of bytes available

Arguments:

    ppv - Pointer to a VOID pointer that recieves the buffer
    pcb - Pointer to a ULONG that recieves the size of the buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSReadFile::GetBuffer(
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

        if (!m_bExtractedAll)
        {
            hr = E_PENDING;
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = m_workFile.GetBuffer(m_cbExtracted, ppv)))
    {
        *pcb = static_cast<ULONG>(m_cbExtracted);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSReadFile::GetFileName

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
CXPSReadFile::GetFileName(
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

/*++

Routine Name:

    CXPSReadFile::AddFilePart

Routine Description:

    This routine adds a PK file name to the stack of files that
    constitute the XPS part

Arguments:

    szFileName - The file name to add
    pPkFile    - The PK file to add to the stack

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSReadFile::AddFilePart(
    _In_ PCSTR    szFileName,
    _In_ CONST IPKFile* pPkFile
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szFileName, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPkFile, E_POINTER)))
    {
        try
        {
            if (m_cstrFileName == szFileName)
            {
                m_partStack.push_back(RecordTracker(pPkFile, FALSE));
            }
            else
            {
                RIP("Filename differs from currently open file\n");

                hr = E_FAIL;
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSReadFile::GetFileParts

Routine Description:

    This routine retrieves the PK files that constitute the XPS part

Arguments:

    ppPartStack - Pointer to an XPSPartStack pointer that recieves the part stack

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSReadFile::GetFileParts(
    _Outptr_ XPSPartStack** ppPartStack
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppPartStack, E_POINTER)))
    {
        *ppPartStack = &m_partStack;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSReadFile::DecompressNextFile

Routine Description:

    This routine decompresses the next compressed part in the part stack

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSReadFile::DecompressNextFile(
    VOID
    )
{
    HRESULT hr = S_OK;

    //
    // If there are more files to decompress
    //
    if (!m_bExtractedAll)
    {
        try
        {
            //
            // Decompress the next unprocessed file into the working buffer
            //
            XPSPartStack::iterator iterXpsStack = m_partStack.begin();

            for (;iterXpsStack != m_partStack.end(); iterXpsStack++)
            {
                if (!iterXpsStack->second)
                {
                    if (SUCCEEDED(hr = Decompress(iterXpsStack->first)))
                    {
                        iterXpsStack->second = TRUE;
                    }

                    break;
                }
            }

            if (iterXpsStack == m_partStack.end())
            {
                //
                // That was the last record
                //
                m_bExtractedAll = TRUE;
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }
    else
    {
        RIP("File has already been fully extracted\n");

        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSReadFile::Decompress

Routine Description:

    This routine uses the PK archive handler to decompress the PK file specified

Arguments:

    pRecord - The PK file to be decompressed

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSReadFile::Decompress(
    _In_ CONST IPKFile* pRecord
    )
{
    HRESULT hr = S_OK;

    ECompressionType eCompType;
    ULONG cbDecompressed = 0;

    if (SUCCEEDED(hr = CHECK_POINTER(pRecord, E_POINTER)) &&
        SUCCEEDED(hr = pRecord->GetDecompressedSize(&cbDecompressed)) &&
        SUCCEEDED(hr = pRecord->GetCompressionMethod(&eCompType)))
    {
        if (eCompType == CompDeflated ||
            eCompType == CompNone)
        {
            PVOID pUnCompressedData = NULL;

            if (SUCCEEDED(hr = m_workFile.GetBuffer(m_cbExtracted + cbDecompressed, &pUnCompressedData)))
            {
                pUnCompressedData = reinterpret_cast<PBYTE>(pUnCompressedData) + m_cbExtracted;

                if (SUCCEEDED(hr = pRecord->DecompressTo(pUnCompressedData, cbDecompressed)))
                {
                    m_cbExtracted += cbDecompressed;
                }
            }
        }
        else
        {
            ERR("Unsupported compression method\n");

            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSReadFile::CopyBuffer

Routine Description:

    This routine copies the decompressed data from the working buffer to
    the specified buffer

Arguments:

    pv      - Pointer to the buffer to recieve the file data
    cb      - The size of the data buffer
    pcbRead - Pointer to a ULONG that recieves the number of bytes written to the buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSReadFile::CopyBuffer(
    _Out_writes_bytes_(cb) void*  pv,
    _In_             ULONG  cb,
    _Out_            ULONG* pcbRead
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pv, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcbRead, E_POINTER)))
    {
        *pcbRead = 0;

        if (cb > 0)
        {
            //
            // We return the smaller of the amount requested and the
            // amount available
            //
            *pcbRead = static_cast<ULONG>(min(m_cbExtracted - m_cbSent, cb));

            PVOID pData = NULL;

            if (SUCCEEDED(hr = m_workFile.GetBufferAt(static_cast<ULONG>(m_cbSent), static_cast<ULONGLONG>(*pcbRead), &pData)))
            {
                CopyMemory(pv, pData, *pcbRead);

                m_cbSent += *pcbRead;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}
