/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsfiler.h

Abstract:

   Definition of the XPS file reader class. This class implements
   ISequentialStream::Read by using the IPKFile interface to supply the
   client with decompressed data as requested. This allows the file to
   be passed directly to a SAX or DOM handler for parsing.

--*/

#pragma once

#include "xps.h"
#include "ipkarch.h"
#include "workbuff.h"
#include "cunknown.h"

class CXPSReadFile : public CUnknown<ISequentialStream>
{
public:
    CXPSReadFile();

    virtual ~CXPSReadFile();

    //
    // ISequentialStream members
    //
    HRESULT STDMETHODCALLTYPE
    Read(
        _Out_writes_bytes_(cb) void*  pv,
        _In_             ULONG  cb,
        _Out_            ULONG* pcbRead
        );

    HRESULT STDMETHODCALLTYPE
    Write(
        _In_reads_bytes_(cb) CONST void*,
        _In_ ULONG cb,
        _Out_opt_ ULONG*
        );

    HRESULT
    Open(
        _In_ PCSTR szFileName
        );

    HRESULT
    Close(
        VOID
        );

    HRESULT
    AddFilePart(
        _In_ PCSTR          szFileName,
        _In_ CONST IPKFile* pPkFile
        );

    HRESULT
    GetFileParts(
        _Outptr_ XPSPartStack** ppPartStack
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
    HRESULT
    DecompressNextFile(
        VOID
        );

    HRESULT
    Decompress(
        _In_ CONST IPKFile* pRecord
        );

    HRESULT
    CopyBuffer(
        _Out_writes_bytes_(cb) void*  pv,
        _In_             ULONG  cb,
        _Out_            ULONG* pcbRead
        );

private:
    CWorkingBuffer            m_workFile;

    CStringXDA                m_cstrFileName;

    XPSPartStack              m_partStack;

    SIZE_T                    m_cbSent;

    SIZE_T                    m_cbExtracted;

    BOOL                      m_bExtractedAll;
};

