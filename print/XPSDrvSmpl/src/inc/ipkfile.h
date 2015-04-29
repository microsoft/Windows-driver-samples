/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ipkfile.h

Abstract:

   Definition of the PK file interface supported by the PK archive handling module.

--*/

#pragma once

//
// All compression types added for completeness. Only CompNone and
// CompDeflated are correctly supported.
//
enum ECompressionType
{
    CompNone = 0,
    CompShrunk,
    CompFactor1,
    CompFactor2,
    CompFactor3,
    CompFactor4,
    CompImploded,
    CompTokenized,
    CompDeflated,
    CompDefalted64,
    CompPKImploded,
    CompPKReserved,
    CompBZIP2
};

class IPKFile
{
public:
    /*++

    Routine Name:

        GetCompressionMethod

    Routine Description:

        This method returns the current compression methof for the archive file

    Arguments:

        pCompType - Pointer to the ECompressionType enumeration that recieves the compression type

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual HRESULT
    GetCompressionMethod(
        _Out_ ECompressionType* peCompType
        ) CONST = 0;

    /*++

    Routine Name:

        GetDecompressedSize

    Routine Description:

        This routine retrieves the size of the uncompressed archive file

    Arguments:

        pcbUnCompressed - Pointer to a ULONG that recieves the decompressed data size

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual HRESULT
    GetDecompressedSize(
        _Out_ ULONG* pcbUnCompressed
        ) CONST = 0;

    /*++

    Routine Name:

        DecompressTo

    Routine Description:

        This routine decompresses the archive file data to the buffer passed

    Arguments:

        pDecompBuffer  - Pointer to the buffer to be filled with decompressed data
        cbDecompBuffer - Size of the decompresssion buffer passed

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual HRESULT
    DecompressTo(
        _Out_writes_bytes_(cbDecompBuffer) PVOID pDecompBuffer,
                                     ULONG cbDecompBuffer
        ) CONST = 0;
};

