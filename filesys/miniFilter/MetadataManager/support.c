/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    operations.c

Abstract:

    This is the support routines module of the kernel mode filter driver implementing
    filter metadata management.


Environment:

    Kernel mode


--*/



#include "pch.h"

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FmmAllocateUnicodeString)
#pragma alloc_text(PAGE, FmmFreeUnicodeString)
#pragma alloc_text(PAGE, FmmTargetIsVolumeOpen)
#pragma alloc_text(PAGE, FmmIsImplicitVolumeLock)
#endif

//
//  Support Routines
//

NTSTATUS
FmmAllocateUnicodeString (
    _Inout_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine allocates a unicode string

Arguments:

    String - supplies the size of the string to be allocated in the MaximumLength field
             return the unicode string

Return Value:

    STATUS_SUCCESS                  - success
    STATUS_INSUFFICIENT_RESOURCES   - failure

--*/
{
    PAGED_CODE();

    String->Buffer = ExAllocatePoolZero( PagedPool,
                                         String->MaximumLength,
                                         FMM_STRING_TAG );

    if (String->Buffer == NULL) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Fmm]: Failed to allocate unicode string of size 0x%x\n",
                    String->MaximumLength) );

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    String->Length = 0;

    return STATUS_SUCCESS;
}

VOID
FmmFreeUnicodeString (
    _Inout_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine frees a unicode string

Arguments:

    String - supplies the string to be freed

Return Value:

    None

--*/
{
    PAGED_CODE();

    ExFreePoolWithTag( String->Buffer,
                       FMM_STRING_TAG );

    String->Length = String->MaximumLength = 0;
    String->Buffer = NULL;
}


BOOLEAN
FmmTargetIsVolumeOpen (
    _In_ PFLT_CALLBACK_DATA Cbd
    )
/*++

Routine Description:

    This routine returns if the target object in this callback datastructure
    is a volume.  If the file object is NULL then assume this is NOT a volume
    file object

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.

Return Value:

    TRUE  - target is a volume
    FALSE - target is not a volume

--*/
{
    PAGED_CODE();

    if ((Cbd->Iopb->TargetFileObject != NULL) &&
        FlagOn( Cbd->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN )) {

         return TRUE;
    } else {

        return FALSE;
    }
}

NTSTATUS
FmmIsImplicitVolumeLock(
    _In_ PFLT_CALLBACK_DATA Cbd,
    _Out_ PBOOLEAN IsLock
    )
/*++

Routine Description:

    This routine determines if an open is a implicit volume lock.

Arguments

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.

    IsLock                - Supplies a pointer to a user allocated boolean
                            which is used to tell the user whether the
                            operation is an implied volume lock.
Return Value:

    Returns STATUS_SUCCESS if the the function determined whether or not
    the operation was a volume lock. On STATUS_SUCCESS it is safe to check
    IsLock to get the answer. Otherwise, the check failed and we don't know
    if it is a lock or not. STATUS_INVALID_PARAMETER indicates that the
    volume's file system type is unrecognized by the check function. This is
    an error code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PFMM_INSTANCE_CONTEXT instanceContext = NULL;
    USHORT shareAccess;

    PAGED_CODE();

    //
    //  Get the instance context so we know
    //  which file system we are attached to.
    //

    status = FltGetInstanceContext( Cbd->Iopb->TargetInstance,
                                    &instanceContext );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmIsImplicitVolumeLock -> Failed to get instance context.\n") );
        goto FmmIsImplicitVolumeLockCleanup;
    }

    FLT_ASSERT( instanceContext != NULL );

    //
    //  Now check to see if the open is an implicit volume lock.  It is if the
    //  caller doesn't want to allow other handles on the volume to be open for
    //  write and/or delete (i.e. they must be read-only).  We must close our
    //  handle to our metadata file so that the volume open can succeed.
    //

    shareAccess = Cbd->Iopb->Parameters.Create.ShareAccess;

#pragma warning( push )
#pragma warning( disable: 4061 )
    switch (instanceContext->FilesystemType) {

        case FLT_FSTYPE_REFS:
        case FLT_FSTYPE_NTFS:
        case FLT_FSTYPE_FAT:
            *IsLock = (!BooleanFlagOn( shareAccess, FILE_SHARE_WRITE | FILE_SHARE_DELETE));
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
    }
#pragma warning( pop )

FmmIsImplicitVolumeLockCleanup:

    if (instanceContext != NULL ) {

        FltReleaseContext( instanceContext );
    }

    return status;
}

