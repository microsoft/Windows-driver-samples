/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsarch.h

Abstract:

   Definition of the XPS archive class. This class is responsible for providing
   an interface to clients that removes the potentially interleaved nature of an
   XPS document. This allows clients to manipulate files using the parts full name
   instead of having to worry about the semantics of interleaved parts.

--*/

#pragma once

#include "ipkarch.h"
#include "xpsfiler.h"

class CXPSArchive
{
public:
    CXPSArchive(
        _In_ IPrintReadStream*  pReadStream,
        _In_ IPrintWriteStream* pWriteStream
        );

    virtual ~CXPSArchive();

    HRESULT
    InitialiseFile(
        _In_z_ PCSTR szFileName
        );

    HRESULT
    GetFileStream(
        _In_z_      PCSTR               szFileName,
        _Outptr_ ISequentialStream** ppFileStream
        );

    HRESULT
    CloseCurrent(
        VOID
        );

    HRESULT
    SendCurrentFile(
        _In_ ECompressionType eCompType
        );

    HRESULT
    SendCurrentFile(
        VOID
        );

    HRESULT
    SendFile(
        _In_z_                PCSTR            szFileName,
        _In_reads_bytes_(cbBuffer) PVOID            pBuffer,
        _In_                  ULONG            cbBuffer,
        _In_                  ECompressionType eCompType
        );

private:
    HRESULT
    AddFile(
        _In_z_ PCSTR szFileName
        );

private:
    HMODULE                    m_hPkArch;

    CComPtr<IPKArchive>        m_pPkArchive;

    CXPSReadFile               m_XpsFile;

    SentList                   m_sentList;

    CComPtr<IPrintWriteStream> m_pWriteStream;
};

