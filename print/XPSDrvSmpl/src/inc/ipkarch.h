/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ipkarch.h

Abstract:

   Definition of the interface supported by the PK archive handling module.

--*/

#pragma once

#include "ipkfile.h"

//
// {5A0F4115-D4D3-401e-8071-A440D6D07092}
//
DEFINE_GUID(CLSID_PKArchiveHandler, 0x5a0f4115, 0xd4d3, 0x401e, 0x80, 0x71, 0xa4, 0x40, 0xd6, 0xd0, 0x70, 0x92);

//
// {BDBBDF56-C742-4efd-8075-AF2C7B247F38}
//
DEFINE_GUID(IID_IPKArchive, 0xbdbbdf56, 0xc742, 0x4efd, 0x80, 0x75, 0xaf, 0x2c, 0x7b, 0x24, 0x7f, 0x38);


typedef map<CStringXDA, CONST IPKFile*> NameIndex;

class IPKArchive : public IUnknown
{
public:
    /*++

    Routine Name:

        SetReadStream

    Routine Description:

        This routine sets the read stream for the PK archive handler

    Arguments:

        pReadStream - Pointer to the print read stream interface

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    virtual SetReadStream(
        _In_ IPrintReadStream* pReadStream
        ) = 0;

    /*++

    Routine Name:

        SetWriteStream

    Routine Description:

        This routine sets the write stream for the PK archive handler

    Arguments:

        pWriteStream - Pointer to the print write stream interface

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    virtual SetWriteStream(
        _In_ IPrintWriteStream* pWriteStream
        ) = 0;

    /*++

    Routine Name:

        ProcessReadStream

    Routine Description:

        This method instructs the PK archive handler to process the PK archive for all records

    Arguments:

        None

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    virtual ProcessReadStream(
        VOID
        ) = 0;

    /*++

    Routine Name:

        GetNameIndex

    Routine Description:

        This routine retrieves the archives indexed by name

    Arguments:

        ppNameIdx - Pointer to a NameIndex pointer that recieves the archive index

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    virtual GetNameIndex(
        _Outptr_ NameIndex** ppNameIdx
        ) = 0;

    /*++

    Routine Name:

        SendFile

    Routine Description:

        This routine sends the PK file to the write stream

    Arguments:

        pFile - Pointer to the IPKFile to be sent to the write stream

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    virtual SendFile(
        _In_ CONST IPKFile* pFile
        ) = 0;

    /*++

    Routine Name:

        SendFile

    Routine Description:

        This routine compresses and sends a buffer as a PK archive to the write stream

    Arguments:

        szFileName      - The name of the archive to be created
        pBuffer         - The buffer containing the uncompressed archive data
        cbBuffer        - The size of the uncompressed archive data
        compressionType - The type of compression to be used (only CompNone and CompDeflated are supported)

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    virtual SendFile(
        _In_z_                PCSTR            szFileName,
        _In_reads_bytes_(cbBuffer) PVOID            pBuffer,
                              ULONG            cbBuffer,
                              ECompressionType eCompType
        ) = 0;

    /*++

    Routine Name:

        Close

    Routine Description:

        This routine closes and finalises the PK archive

    Arguments:

        None

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    virtual Close(
        VOID
        ) = 0;
};

