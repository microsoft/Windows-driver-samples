/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    MetadataManagerProc.h

Abstract:

    This is the header file defining the functions of the kernel mode
    filter driver implementing filter metadata management.


Environment:

    Kernel mode


--*/

#define MAKE_RESOURCE_OWNER(X) (((ERESOURCE_THREAD)(X)) | 0x3)

//
//  Functions implemented in operations.c
//

FLT_PREOP_CALLBACK_STATUS
FmmPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
FmmPostCreate (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FmmPreCleanup (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
FmmPostCleanup (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );


FLT_PREOP_CALLBACK_STATUS
FmmPreFSControl (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
FmmPostFSControl (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FmmPreDeviceControl (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
FmmPostDeviceControl (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FmmPreShutdown (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
FmmPrePnp (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
FmmPostPnp (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

//
//  Functions implemented in datastore.c
//

_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(InstanceContext->MetadataResource)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FmmOpenMetadata (
    _In_ PFMM_INSTANCE_CONTEXT InstanceContext,
    _In_ BOOLEAN CreateIfNotPresent
    );

_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(InstanceContext->MetadataResource)    
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FmmCloseMetadata (
    _In_ PFMM_INSTANCE_CONTEXT InstanceContext
    );

NTSTATUS
FmmReleaseMetadataFileReferences (
    _Inout_ PFLT_CALLBACK_DATA Cbd
    );

NTSTATUS
FmmReacquireMetadataFileReferences (
    _Inout_ PFLT_CALLBACK_DATA Cbd
    );

NTSTATUS
FmmSetMetadataOpenTriggerFileObject (
    _Inout_ PFLT_CALLBACK_DATA Cbd
    );

_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(InstanceContext->MetadataResource)
_Releases_lock_(InstanceContext->MetadataResource)
_IRQL_requires_max_(APC_LEVEL)
VOID
FmmBeginFileSystemOperation (
    IN PFMM_INSTANCE_CONTEXT InstanceContext
    );

_Acquires_lock_(_Global_critical_region_)
_Requires_lock_not_held_(InstanceContext->MetadataResource)    
_Acquires_exclusive_lock_(InstanceContext->MetadataResource)
_IRQL_requires_max_(APC_LEVEL)
VOID
FmmEndFileSystemOperation (
    IN PFMM_INSTANCE_CONTEXT InstanceContext
    );


#if VERIFY_METADATA_OPENED
    
NTSTATUS
FmmIsMetadataOpen (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _Out_ BOOLEAN* MetadataOpen
    );

#endif

//
//  Functions implemented in support.c
//

NTSTATUS
FmmAllocateUnicodeString (
    _Inout_ PUNICODE_STRING String
    );

VOID
FmmFreeUnicodeString (
    _Inout_ PUNICODE_STRING String
    );

BOOLEAN
FmmTargetIsVolumeOpen (
    _In_ PFLT_CALLBACK_DATA Cbd
    );

NTSTATUS
FmmIsImplicitVolumeLock( 
    _In_ PFLT_CALLBACK_DATA Cbd,
    _Out_ PBOOLEAN IsLock
    );

//
//  Lock primitives
//

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
VOID
FmmAcquireResourceExclusive (
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_exclusive_lock_(*_Curr_)
    PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
               !ExIsResourceAcquiredSharedLite(Resource));

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceExclusiveLite( Resource, TRUE );
}

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
VOID
FmmAcquireResourceShared (
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_shared_lock_(*_Curr_)
    PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceSharedLite( Resource, TRUE );
}

_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
VOID
FmmReleaseResource (
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) 
    PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
               ExIsResourceAcquiredSharedLite(Resource));

    ExReleaseResourceLite(Resource);
    KeLeaveCriticalRegion();
}



