/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsfilew.h

Abstract:

   Definition of an XPS file writer. This implements ISequentialStream::Write
   and essentially wraps a buffer that recieves and stores the part information
   so that in can be later compressed and written out.

--*/

#pragma once

#include "workbuff.h"
#include "cunknown.h"
#include "xdstring.h"

class CXPSWriteFile : public CUnknown<ISequentialStream>
{
public:
    CXPSWriteFile();

    CXPSWriteFile(
        PCSTR szFileName
        );

    virtual ~CXPSWriteFile();

    //
    // ISequentialStream members
    //
    HRESULT STDMETHODCALLTYPE
    Read(
        _Out_writes_bytes_to_(cb, *pcbRead) void*,
        _In_ ULONG cb,
        _Out_opt_ ULONG* pcbRead
        );

    HRESULT STDMETHODCALLTYPE
    Write(
        _In_reads_bytes_(cbData) CONST void* pData,
        _In_                ULONG       cbData,
        _Out_               ULONG*      pcbWritten
        );

    HRESULT
    GetBuffer(
        _Outptr_result_bytebuffer_(*pcb) PVOID* ppv,
        _Out_                    ULONG* pcb
        );

    HRESULT
    GetFileName(
        _Outptr_ PSTR* pszFileName
        );

private:
    CWorkingBuffer m_workFile;

    ULONG          m_cbWritten;

    CStringXDA       m_cstrFileName;
};

