/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    csvfs.h

Abstract:

    This module contains the scan interface for AV filter to call.

Environment:

    Kernel mode

--*/
#ifndef __CSVFS_H__
#define __CSVFS_H__


NTSTATUS
AvPreCleanupCsvfs (
    _Unreferenced_parameter_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PAV_STREAM_CONTEXT StreamContext,
    _Out_ BOOLEAN *UpdateRevisionNumbers,
    _Out_ LONGLONG *VolumeRevisionPtr,
    _Out_ LONGLONG *CacheRevisionPtr,
    _Out_ LONGLONG *FileRevisionPtr               
    );

NTSTATUS
AvPostCreateCsvfs (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PAV_STREAM_CONTEXT StreamContext,
    _Out_ BOOLEAN *UpdateRevisionNumbers,
    _Out_ LONGLONG *VolumeRevisionPtr,
    _Out_ LONGLONG *CacheRevisionPtr,
    _Out_ LONGLONG *FileRevisionPtr               
    );

NTSTATUS
AvPreCreateCsvfs (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects
    );

BOOLEAN
AvIsCsvDlEcpPresent (
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_CALLBACK_DATA Data
     );

BOOLEAN
AvIsVolumeOnCsvDisk (
    _In_ PFLT_VOLUME Volume
    );

#endif

