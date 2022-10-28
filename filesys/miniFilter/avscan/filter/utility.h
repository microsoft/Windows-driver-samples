/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    utility.h

Abstract:

    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel. The functions include
    generic table routines.

Environment:

    Kernel mode

--*/
#ifndef __UTILITY_H__
#define __UTILITY_H__

#define AV_STRING_TAG                        'tSvA'
#define AV_RESOURCE_TAG                      'cRvA'
#define AV_KEVENT_TAG                        'eKvA'
#define AV_TABLE_ENTRY_TAG                   'eTvA'

//////////////////////////////////////////////////////////////////////////////
//  ReFS Compatibility Helpers                                              //
//////////////////////////////////////////////////////////////////////////////

//
//  This helps us deal with ReFS 128-bit file IDs and NTFS 64-bit file IDs.
//

#define AV_INVALID_FILE_REFERENCE( _fileid_ ) \
    (((_fileid_).FileId64.UpperZeroes == 0ll) && \
     ((_fileid_).FileId64.Value == (ULONGLONG)FILE_INVALID_FILE_ID))

#define AV_SET_INVALID_FILE_REFERENCE( _fileid_ ) \
      (_fileid_).FileId64.UpperZeroes = 0ll;\
      (_fileid_).FileId64.Value = (ULONGLONG)FILE_INVALID_FILE_ID;

typedef union _AV_FILE_REFERENCE {

    struct {
        ULONGLONG   Value;
        ULONGLONG   UpperZeroes;
    } FileId64;

    FILE_ID_128     FileId128;

} AV_FILE_REFERENCE, *PAV_FILE_REFERENCE;


//
//  The generic table entry data structure.
//

typedef struct _AV_GENERIC_TABLE_ENTRY {

    AV_FILE_REFERENCE FileId;
    ULONG      InfectedState;

    //
    // Revision numbers for files on CSVFS
    //
    LONGLONG   VolumeRevision;
    LONGLONG   CacheRevision;
    LONGLONG   FileRevision;

} AV_GENERIC_TABLE_ENTRY, *PAV_GENERIC_TABLE_ENTRY;

#define AV_GENERIC_TABLE_ENTRY_SIZE         sizeof( AV_GENERIC_TABLE_ENTRY )

/*
_IRQL_requires_same_
_Function_class_(RTL_GENERIC_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS
AvCompareEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );

_IRQL_requires_same_
__drv_allocatesMem(Mem)
_Function_class_(RTL_GENERIC_ALLOCATE_ROUTINE)
PVOID
NTAPI
AvAllocateGenericTableEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ CLONG ByteSize
    );

_IRQL_requires_same_
_Function_class_(RTL_GENERIC_FREE_ROUTINE)
VOID
NTAPI
AvFreeGenericTableEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Entry
    );
*/

RTL_GENERIC_COMPARE_ROUTINE AvCompareEntry;

RTL_GENERIC_ALLOCATE_ROUTINE AvAllocateGenericTableEntry;

RTL_GENERIC_FREE_ROUTINE AvFreeGenericTableEntry;

//
// NTFS supports a file state cache. Since CSVFS is built on top of
// NTFS, it can also support the cache.
//
#define FS_SUPPORTS_FILE_STATE_CACHE(VolumeFilesystemType) \
  ( ((VolumeFilesystemType) == FLT_FSTYPE_NTFS) || \
    ((VolumeFilesystemType) == FLT_FSTYPE_CSVFS) || \
    ((VolumeFilesystemType) == FLT_FSTYPE_REFS) )


FORCEINLINE
PERESOURCE
AvAllocateResource (
    VOID
    )
{
    //
    //  eResource by its rule has to be in the non-paged pool
    //  NonPagedPoolNx: non-executable non-paged pool
    //

    return ExAllocatePoolZero( NonPagedPoolNx,
                               sizeof( ERESOURCE ),
                               AV_RESOURCE_TAG );
}

FORCEINLINE
VOID
AvFreeResource (
    _In_ PERESOURCE Resource
    )
{

    ExFreePoolWithTag( Resource,
                       AV_RESOURCE_TAG );
}

FORCEINLINE
PKEVENT
AvAllocateKevent (
    VOID
    )
{
    //
    //  KEVENT has to be in the non-paged pool
    //

    return ExAllocatePoolZero( NonPagedPoolNx,
                               sizeof( KEVENT ),
                               AV_KEVENT_TAG );
}

FORCEINLINE
VOID
AvFreeKevent (
    _In_ PKEVENT Event
    )
{

    ExFreePoolWithTag( Event,
                       AV_KEVENT_TAG );
}

NTSTATUS
AvGetFileId (
    _In_    PFLT_INSTANCE Instance,
    _In_    PFILE_OBJECT FileObject,
    _Out_   PAV_FILE_REFERENCE FileId
    );

NTSTATUS
AvGetFileSize (
    _In_    PFLT_INSTANCE Instance,
    _In_    PFILE_OBJECT FileObject,
    _Out_   PLONGLONG Size
    );

NTSTATUS
AvGetFileEncrypted (
    _In_   PFLT_INSTANCE Instance,
    _In_   PFILE_OBJECT FileObject,
    _Out_  PBOOLEAN  Encrypted
    );

LONG
AvExceptionFilter (
    _In_ PEXCEPTION_POINTERS ExceptionPointer,
    _In_ BOOLEAN AccessingUserBuffer
    );

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
AvAcquireResourceExclusive (
    _Inout_ _Acquires_exclusive_lock_(*Resource) PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
               !ExIsResourceAcquiredSharedLite(Resource));

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceExclusiveLite( Resource, TRUE );
}

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
AvAcquireResourceShared (
    _Inout_ _Acquires_shared_lock_(*Resource) PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceSharedLite( Resource, TRUE );
}

FORCEINLINE
VOID
_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(_Global_critical_region_)
AvReleaseResource (
    _Inout_ _Requires_lock_held_(*Resource) _Releases_lock_(*Resource) PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
               ExIsResourceAcquiredSharedLite(Resource));

    ExReleaseResourceLite(Resource);
    KeLeaveCriticalRegion();
}

#define LIST_FOR_EACH_SAFE(curr, n, head) \
        for (curr = (head)->Flink , n = curr->Flink ; curr != (head); \
             curr = n, n = curr->Flink )

#endif

