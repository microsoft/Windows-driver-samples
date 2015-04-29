/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    strings.h

Abstract:

    This module contains the public function definitions for the routines
    in strings.c that handle conversion of integer/data buffer to/from 
    string represenation

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#ifndef __STRINGS_H__
#define __STRINGS_H__

VOID
Strings_CreateDataBufferString(
    _In_reads_bytes_(DataBufferLength) PCHAR    DataBuffer,
    _In_  ULONG    DataBufferLength,
    _In_  ULONG    NumBytesToDisplay,
    _In_  ULONG    DisplayBlockSize,
    _Outptr_result_maybenull_ LPSTR  *BufferString
);

_When_(*nUnsigneds == 0, _At_(*UnsignedList, _Post_null_))
_When_(*nUnsigneds > 0, _At_(*UnsignedList, _Post_notnull_))
_Success_(return != FALSE)
BOOL
Strings_StringToUnsignedList(
    _Inout_ LPSTR   InString,
    _In_    ULONG   UnsignedSize,
    _In_    ULONG   Base,
    _Outptr_result_bytebuffer_maybenull_(*nUnsigneds) PCHAR *UnsignedList,
    _Out_   PULONG  nUnsigneds
);

#endif
