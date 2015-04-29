/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   streamcnv.h

Abstract:

    This wrapper class presents an IStream interface to an IPrintReadStream object.
    This is useful for where an IStream interface is required for an API but is not
    available without creating a seperate buffer and copying data in, e.g. decoding
    bitmap data in WIC. This is not appropriate for use with interfaces that write
    back into the stream.

--*/

#pragma once

#include "cunknown.h"

class CPrintReadStreamToIStream : public CUnknown<IStream>
{
public:
    /*++

    Routine Name:

        CPrintReadStreamToIStream

    Routine Description:

        CPrintReadStreamToIStream constructor

    Arguments:

        pReadStream - the print read stream to wrap with an IStream interface

    Return Value:

        None

    --*/
    CPrintReadStreamToIStream(
        _In_ IPrintReadStream* pReadStream
        ) :
        CUnknown<IStream>(IID_IStream),
        m_pReadStream(pReadStream)
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CoFileTimeNow(&m_mTime)) &&
            SUCCEEDED(hr = CoFileTimeNow(&m_cTime)) &&
            SUCCEEDED(hr = CoFileTimeNow(&m_aTime)))
        {
            m_cbStream.QuadPart = 0;
            if (m_pReadStream == NULL)
            {
                hr = E_POINTER;
            }
        }

        if (FAILED(hr))
        {
            throw CXDException(hr);
        }
    }

    /*++

    Routine Name:

        ~CPrintReadStreamToIStream

    Routine Description:

        CPrintReadStreamToIStream destructor

    Arguments:

        None

    Return Value:

        None

    --*/
    virtual ~CPrintReadStreamToIStream()
    {
    }

    //
    // ISequentialStream methods
    //
    /*++

    Routine Name:

        Read

    Routine Description:

        Implements IStream::Read by calling on to IPrintReadStream::ReadBytes

    Arguments:

        pv      - Pointer to the buffer to read into
        cb      - Count of bytes avaiable in the buffer pointed to by pv
        pcbRead - pointer to a ULONG that recieves the count of bytes actually read

    Return Value:

        HRESULT
        S_OK    - On success and *pcbRead == cb
        S_FALSE - On success and *pcbRead < cb
        E_*     - On error

    --*/
    HRESULT STDMETHODCALLTYPE
    Read(
        _Out_writes_bytes_to_(cb, *pcbRead) PVOID  pv,
        _In_ ULONG  cb,
        _Out_opt_ PULONG pcbRead
        )
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pv, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pcbRead, E_POINTER)) &&
            SUCCEEDED(hr = CoFileTimeNow(&m_aTime)))
        {
            BOOL bEOF = FALSE;

            *pcbRead = 0;

            while (SUCCEEDED(hr) &&
                    !bEOF &&
                    *pcbRead < cb)
            {
                DWORD cbRead = 0;

                hr = m_pReadStream->ReadBytes(reinterpret_cast<PVOID>(reinterpret_cast<PBYTE>(pv) + *pcbRead), cb - *pcbRead, &cbRead, &bEOF);

                if (SUCCEEDED(hr))
                {
                    *pcbRead += cbRead;
                }
            }
        }

        if (SUCCEEDED(hr) &&
            *pcbRead < cb)
        {
            hr = S_FALSE;
        }

        ERR_ON_HR(hr);
        return hr;
    }

    /*++

    Routine Name:

        Write

    Routine Description:

        Not implemented

    Arguments:

        None

    Return Value:

        E_NOTIMPL

    --*/
    HRESULT STDMETHODCALLTYPE
    Write(
        _In_reads_bytes_(cb) CONST VOID*,
        _In_ ULONG cb,
        _Out_opt_ PULONG
        )
    {
        UNREFERENCED_PARAMETER(cb);
        ERR("Unsupported method called.\n");
        return E_NOTIMPL;
    }

    //
    // IStream methods
    //
    /*++

    Routine Name:

        Seek

    Routine Description:

        Implements IStream::Seek by calling on to IPrintReadStream::Seek

    Arguments:

        dlibMove        - Count of bytes to displace the current position relative to the dwOrigin parameter
        dwOrigin        - The origin to apply the displacement from
        plibNewPosition - Pointer to a variable to recieve the new position of the seek pointer

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT STDMETHODCALLTYPE
    Seek(
        LARGE_INTEGER   dlibMove,
        DWORD           dwOrigin,
        _Out_opt_ ULARGE_INTEGER* plibNewPosition
        )
    {
        HRESULT hr = S_OK;

        ULONGLONG libNewPos = 0;
        if (SUCCEEDED(hr = m_pReadStream->Seek(dlibMove.QuadPart, dwOrigin, &libNewPos)) &&
            plibNewPosition != NULL)
        {
            plibNewPosition->QuadPart = libNewPos;
        }

        ERR_ON_HR(hr);
        return hr;
    }

    /*++

    Routine Name:

        SetSize

    Routine Description:

        Not implemented

    Arguments:

        None

    Return Value:

        E_NOTIMPL

    --*/
    HRESULT STDMETHODCALLTYPE
    SetSize(
        ULARGE_INTEGER
        )
    {
        ERR("Unsupported method called.\n");
        return E_NOTIMPL;
    }

    /*++

    Routine Name:

        CopyTo

    Routine Description:

        Not implemented

    Arguments:

        None

    Return Value:

        E_NOTIMPL

    --*/
    HRESULT STDMETHODCALLTYPE
    CopyTo(
        _In_ IStream*,
        ULARGE_INTEGER,
        _Out_opt_ ULARGE_INTEGER*,
        _Out_opt_ ULARGE_INTEGER*
        )
    {
        ERR("Unsupported method called.\n");
        return E_NOTIMPL;
    }

    /*++

    Routine Name:

        Commit

    Routine Description:

        Not implemented

    Arguments:

        None

    Return Value:

        E_NOTIMPL

    --*/
    HRESULT STDMETHODCALLTYPE
    Commit(
        DWORD
        )
    {
        ERR("Unsupported method called.\n");
        return E_NOTIMPL;
    }

    /*++

    Routine Name:

        Revert

    Routine Description:

        Not implemented

    Arguments:

        None

    Return Value:

        E_NOTIMPL

    --*/
    HRESULT STDMETHODCALLTYPE
    Revert(
        VOID
        )
    {
        ERR("Unsupported method called.\n");
        return E_NOTIMPL;
    }

    /*++

    Routine Name:

        LockRegion

    Routine Description:

        Not implemented

    Arguments:

        None

    Return Value:

        E_NOTIMPL

    --*/
    HRESULT STDMETHODCALLTYPE
    LockRegion(
        ULARGE_INTEGER,
        ULARGE_INTEGER,
        DWORD
        )
    {
        ERR("Unsupported method called.\n");
        return E_NOTIMPL;
    }

    /*++

    Routine Name:

        UnlockRegion

    Routine Description:

        Not implemented

    Arguments:

        None

    Return Value:

        E_NOTIMPL

    --*/
    HRESULT STDMETHODCALLTYPE
    UnlockRegion(
        ULARGE_INTEGER,
        ULARGE_INTEGER,
        DWORD
        )
    {
        ERR("Unsupported method called.\n");
        return E_NOTIMPL;
    }

    /*++

    Routine Name:

        Stat

    Routine Description:

        Retrieves the STATSG structure for this stream

    Arguments:

        pstatstg    - Pointer to the STATSG structure to be completed
        grfStatFlag - Flag determining what should be completed

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT STDMETHODCALLTYPE Stat(
        STATSTG* pstatstg,
        DWORD    grfStatFlag
        )
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pstatstg, E_POINTER)) &&
            SUCCEEDED(hr = GetStreamSize(&pstatstg->cbSize)))
        {
            if (grfStatFlag == STATFLAG_DEFAULT)
            {
                CComBSTR bstrStoreName(L"PrintReadStream");
                UINT cchStoreName = bstrStoreName.Length();

                pstatstg->pwcsName = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((cchStoreName + 1) * sizeof(BSTR)));

                if (pstatstg->pwcsName != NULL)
                {
                    CopyMemory(pstatstg->pwcsName, bstrStoreName.m_str, cchStoreName * sizeof(BSTR));
                    pstatstg->pwcsName[cchStoreName] = 0;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                pstatstg->pwcsName = NULL;
            }

            pstatstg->type              = STGTY_STREAM;
            pstatstg->mtime             = m_mTime;
            pstatstg->ctime             = m_cTime;
            pstatstg->atime             = m_aTime;
            pstatstg->grfMode           = STGM_READ | STGM_SHARE_DENY_WRITE;
            pstatstg->grfLocksSupported = LOCK_WRITE;
            pstatstg->clsid             = CLSID_NULL;
            pstatstg->grfStateBits      = 0;
        }

        ERR_ON_HR(hr);
        return hr;
    }

    /*++

    Routine Name:

        Clone

    Routine Description:

        Not implemented

    Arguments:

        None

    Return Value:

        E_NOTIMPL

    --*/
    HRESULT STDMETHODCALLTYPE
    Clone(
        IStream**
        )
    {
        ERR("Unsupported method called.\n");
        return E_NOTIMPL;
    }

private:
    /*++

    Routine Name:

        GetStreamSize

    Routine Description:

        Retrieves the stream size. If this is the first time the stream size is accessed
        the method retrieves teh size by seeking to the end of the stream and reporting
        the offset from the start. This value is then stored and used in subsequent
        calls to this method

    Arguments:

        pcbSize - Pointer to the size to be filled out

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    GetStreamSize(
        ULARGE_INTEGER* pcbSize
        )
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pcbSize, E_POINTER)))
        {
            //
            // This stream implementation can never shrink, so if the size is greater
            // than 0 we do not need to re-acquire.
            //
            if (m_cbStream.QuadPart > 0)
            {
                *pcbSize = m_cbStream;
            }
            else
            {
                //
                // Seek to the end of the stream to find the size then reset to the current position
                //
                ULONGLONG libOldPosition = 0;
                if (SUCCEEDED(hr = m_pReadStream->Seek(0, STREAM_SEEK_CUR, &libOldPosition)) &&
                    SUCCEEDED(hr = m_pReadStream->Seek(0, STREAM_SEEK_END, &pcbSize->QuadPart)) &&
                    SUCCEEDED(hr = m_pReadStream->Seek(libOldPosition, STREAM_SEEK_SET, NULL)))
                {
                    //
                    // Store the result
                    //
                    m_cbStream.QuadPart = pcbSize->QuadPart;
                }
            }
        }

        ERR_ON_HR(hr);
        return hr;
    }


private:
    CComPtr<IPrintReadStream> m_pReadStream;

    ULARGE_INTEGER            m_cbStream;

    FILETIME                  m_mTime;

    FILETIME                  m_cTime;

    FILETIME                  m_aTime;
};

