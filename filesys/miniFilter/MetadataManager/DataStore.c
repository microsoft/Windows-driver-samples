/*++

Copyright (c) 2002 - 2003  Microsoft Corporation

Module Name:

    datastore.c

Abstract:

    This module contains routines that provide support for storage and
    retrieval of the filter metadata manager filter metadata.


Environment:

    Kernel mode


--*/

#include "pch.h"

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FmmOpenMetadata)
#pragma alloc_text(PAGE, FmmCloseMetadata)
#pragma alloc_text(PAGE, FmmReleaseMetadataFileReferences)
#pragma alloc_text(PAGE, FmmReacquireMetadataFileReferences)
#pragma alloc_text(PAGE, FmmSetMetadataOpenTriggerFileObject)
#pragma alloc_text(PAGE, FmmBeginFileSystemOperation)
#pragma alloc_text(PAGE, FmmEndFileSystemOperation)
#endif

_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(InstanceContext->MetadataResource)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FmmOpenMetadata (
    _In_ PFMM_INSTANCE_CONTEXT InstanceContext,
    _In_ BOOLEAN CreateIfNotPresent
    )
/*++

Routine Description:

    This routine opens or creates the Fmm metadata on the specified instance.

Arguments:

    InstanceContext     - Supplies the instance context for this instance.
    CreateIfNotPresent  - Supplies if the directory entry must be created if it is not present

Return Value:

    Returns the status of this operation.

Note:

    The caller must hold the instance context resource exclusive when this routine is called.

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING fileName;
    NTSTATUS status;
    ULONG length;

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                ("[Fmm]: Opening metadata file ... (Volume = %p, CreateIfNotPresent = %X)\n",
                 InstanceContext->Volume,
                 CreateIfNotPresent) );

    status = STATUS_SUCCESS;
    fileName.Buffer = NULL;

    //
    //  Get the volume name and construct the full metadata filename.
    //


    length = FMM_DEFAULT_VOLUME_NAME_LENGTH + FMM_METADATA_FILE_NAME_LENGTH;

#pragma warning(push)
#pragma warning(disable:4127) //  Conditional expression is constant
    while (TRUE) {

#pragma warning(pop)

        fileName.MaximumLength = (USHORT)length;

        status = FmmAllocateUnicodeString( &fileName );

        if (!NT_SUCCESS( status )) {

            goto FmmOpenMetadataCleanup;
        }

        status = FltGetVolumeName( InstanceContext->Volume, &fileName, &length );

        if (NT_SUCCESS( status )) {

            status = RtlAppendUnicodeToString( &fileName, FMM_METADATA_FILE_NAME );

            if (NT_SUCCESS( status )) {

                break;
            }
        } else {

            DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS | DEBUG_TRACE_ERROR,
                        ("[Fmm]: Failed to get volume name (Volume = %p, Status = 0x%x)\n",
                         InstanceContext->Volume,
                         status) );
        }


        if (status != STATUS_BUFFER_TOO_SMALL) {

            goto FmmOpenMetadataCleanup;;
        }

        //
        //  Free the filename buffer since a bigger one will be allocated
        //  above
        //

        FmmFreeUnicodeString( &fileName );

        length += FMM_METADATA_FILE_NAME_LENGTH;
    }


    //
    //  Initialize the object attributes and open the file.
    //

    InitializeObjectAttributes( &objectAttributes,
                                &fileName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );




RetryFltCreateFile:


    DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                ("[Fmm]: Calling FltCreateFile for metadata file %wZ (Volume = %p, Status = 0x%x)\n",
                 &fileName,
                 InstanceContext->Volume,
                 status) );


    //
    //  Mark the beginning of a file system operation
    //

    FmmBeginFileSystemOperation( InstanceContext );

    status = FltCreateFile( Globals.Filter,
                            InstanceContext->Instance,
                            &InstanceContext->MetadataHandle,
                            FILE_ALL_ACCESS,
                            &objectAttributes,
                            &ioStatus,
                            (PLARGE_INTEGER) NULL,
                            FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                            FILE_SHARE_READ,
                            (CreateIfNotPresent ? FILE_OPEN_IF : FILE_OPEN),
                            0L,
                            NULL,
                            0L,
                            0 );

    //
    //  Mark the end of a file system operation
    //

    FmmEndFileSystemOperation( InstanceContext );


    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Fmm]: FltCreateFile failure for metadata file %wZ (Volume = %p, Status = 0x%x)\n",
                     &fileName,
                     InstanceContext->Volume,
                     status) );

        if (CreateIfNotPresent && (status == STATUS_OBJECT_PATH_NOT_FOUND)) {

            //
            //  We need to create the metadata file and the creation failed
            //  because the SystemVolumeInformation folder does not exist.
            //  So, create the folder and try again.
            //

            DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                        ("[Fmm]: Creating SystemVolumeInformation folder for metadata file %wZ (Volume = %p, Status = 0x%x)\n",
                         &fileName,
                         InstanceContext->Volume,
                         status) );


            //
            //  Mark the beginning of a file system operation
            //

            FmmBeginFileSystemOperation( InstanceContext );

            status = FltCreateSystemVolumeInformationFolder( InstanceContext->Instance );

            //
            //  Mark the end of a file system operation
            //

            FmmEndFileSystemOperation( InstanceContext );



            if (NT_SUCCESS( status )) {

                //
                //  We have sucessfully created the SystemVolumeInformation folder
                //  Try to create the metadata file again
                //

                goto RetryFltCreateFile;
            } else {

                DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS | DEBUG_TRACE_ERROR,
                            ("[Fmm]: FltCreateSystemVolumeInformationFolder failure for metadata file %wZ (Volume = %p, Status = 0x%x)\n",
                             &fileName,
                             InstanceContext->Volume,
                             status) );
            }
        }

        goto FmmOpenMetadataCleanup;
    }

    //
    //  Retrieve the FileObject from the handle created
    //

    status = ObReferenceObjectByHandle( InstanceContext->MetadataHandle,
                                        STANDARD_RIGHTS_REQUIRED,
                                        *IoFileObjectType,
                                        KernelMode,
                                        &InstanceContext->MetadataFileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Fmm]: Failure to get file object from handle for metadata file %wZ (Volume = %p, Status = 0x%x)\n",
                     &fileName,
                     InstanceContext->Volume,
                     status) );

        goto FmmOpenMetadataCleanup;
    }

    if (ioStatus.Information == FILE_CREATED) {

        //
        //  New metadata was created
        //

        DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: Created new metadata file %wZ (Volume = %p, Status = 0x%x)\n",
                     &fileName,
                     InstanceContext->Volume,
                     status) );

        //
        //  The filter may want to do some initialization on the newly created
        //  metadata file here like adding a header to the file
        //

    }
    else {

        //
        //  Existing metadata was opened
        //

        DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: Opened existing metadata file %wZ (Volume = %p, Status = 0x%x)\n",
                     &fileName,
                     InstanceContext->Volume,
                     status) );

        //
        //  The filter may want to do some sanity checks on the metadata file here
        //  like validating the header of the file
        //

    }

    //
    //  Here the filter may read the metadata contents and initialize
    //  its in memory data structures with the data from the metadata
    //  file
    //


FmmOpenMetadataCleanup:

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Fmm]: Failed to open metadata (Volume = %p, Status = 0x%x)\n",
                     InstanceContext->Volume,
                     status) );

        //
        //  CLose the handle and dereference the file object
        //

        if (InstanceContext->MetadataHandle) {


            //
            //  Mark the beginning of a file system operation
            //

            FmmBeginFileSystemOperation( InstanceContext );

            FltClose( InstanceContext->MetadataHandle );

            //
            //  Mark the end of a file system operation
            //

            FmmEndFileSystemOperation( InstanceContext );

            InstanceContext->MetadataHandle = NULL;

            if (InstanceContext->MetadataFileObject) {

                ObDereferenceObject( InstanceContext->MetadataFileObject );
                InstanceContext->MetadataFileObject = NULL;
            }
        }
    } else {

        DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: Metadata successfully opened (Volume = %p)\n",
                     InstanceContext->Volume) );

        //
        //  Set flags to indicate successful open of filter metadata
        //

        SetFlag( InstanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED );

    }

    if (fileName.Buffer != NULL) {

        FmmFreeUnicodeString( &fileName );
    }

    return status;
}


_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(InstanceContext->MetadataResource)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FmmCloseMetadata (
    _In_ PFMM_INSTANCE_CONTEXT InstanceContext
    )
/*++

Routine Description:

    This routine closes the filters handle to the metadata file.

Arguments:

    InstanceContext - Instance context for this instance.

Return Value:

    Void.

Note:

    The caller must hold the instance context resource when this routine is called.


--*/
{
    PAGED_CODE();

    FLT_ASSERT( InstanceContext->MetadataHandle );
    FLT_ASSERT( InstanceContext->MetadataFileObject );

    DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                ("[Fmm]: Closing metadata file ... (Volume = %p)\n",
                 InstanceContext->Volume ) );

    //
    //  Dereference the file object and close the file handle.
    //

    ObDereferenceObject( InstanceContext->MetadataFileObject );

    InstanceContext->MetadataFileObject = NULL;


    //
    //  Mark the beginning of a file system operation
    //

    FmmBeginFileSystemOperation( InstanceContext );

    FltClose( InstanceContext->MetadataHandle );

    //
    //  Mark the end of a file system operation
    //

    FmmEndFileSystemOperation( InstanceContext );


    InstanceContext->MetadataHandle = NULL;

    //
    //  Reset flag to indicate filter metadata is closed
    //

    ClearFlag( InstanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED );

}

NTSTATUS
FmmReleaseMetadataFileReferences (
    _Inout_ PFLT_CALLBACK_DATA Cbd
    )
/*++

Routine Description:

    This routine releases all references to the metadata file on the specified instance.

Arguments:

    Cbd                 - Supplies a pointer to the callbackData which
                          declares the requested operation.

Return Value:

    Status

Note:

    This routine takes care of the synchronization needed to access the metadata
    file object and handle

    This routine will also set the MetadataOpenTriggerFileObject in the instance context
    to the file object of the volume that triggered the release of the metadata file
    references.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PFMM_INSTANCE_CONTEXT instanceContext = NULL;

    PAGED_CODE();

    //
    //  Get the instance context
    //

    status = FltGetInstanceContext( Cbd->Iopb->TargetInstance,
                                    &instanceContext );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmReleaseMetadataFileReferences -> Failed to get instance context.\n") );

        goto FmmReleaseMetadataFileReferencesCleanup;
    }

    //
    //  Acquire exclusive access to the instance context
    //

    FmmAcquireResourceExclusive( &instanceContext->MetadataResource );

    if (FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION)) {

        //
        //  If this instance context is in a transition state, it implies that
        //  the instance context lock has been released while sending an operation
        //  down to the file system. The reason for doing so is to prevent a potential
        //  deadlock if an underlying filter sends an IO to the top of the filter
        //  stack while we are holding the resource
        //
        //  We have managed to acquire this resource in this state of transition.
        //  It would be incorrect to use or modify the instance context in any way
        //  in this situation. So we simply let go.
        //

        status = STATUS_FILE_LOCK_CONFLICT;

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmReleaseMetadataFileReferences -> Failed to get exclusive access to instance context since it is in a state of transition.\n") );
    } else {

        //
        //  Close the metadata file if it is open
        //

        if (FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED )) {

            DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                        ("[Fmm]: FmmReleaseMetadataFileReferences -> Releasing references to metadata handle and file object (InstanceContext = %p VolumeFileObject = %p)\n",
                         instanceContext,
                         Cbd->Iopb->TargetFileObject) );

            //
            //  Close the metadata file object
            //

            FmmCloseMetadata( instanceContext );

            //
            //  Save the volume file object for which we are releasing our references
            //

            instanceContext->MetadataOpenTriggerFileObject = Cbd->Iopb->TargetFileObject;
        } else {

            DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                        ("[Fmm]: FmmReleaseMetadataFileReferences -> Exit without attempting to release references to metadata handle and file object (InstanceContext = %p, VolumeFileObject = %p, MetadataOpenTriggerFileObject = %p, MetadataAlreadyOpen = 0x%x)\n",
                         instanceContext,
                         Cbd->Iopb->TargetFileObject,
                         instanceContext->MetadataOpenTriggerFileObject,
                         FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED )) );
        }
    }

    //
    //  Relinquish exclusive access to the instance context
    //

    FmmReleaseResource( &instanceContext->MetadataResource );


FmmReleaseMetadataFileReferencesCleanup:

    //
    // Release the references we have acquired
    //

    if (instanceContext != NULL) {

        FltReleaseContext( instanceContext );
    }


    return status;
}


NTSTATUS
FmmReacquireMetadataFileReferences (
    _Inout_ PFLT_CALLBACK_DATA Cbd
    )
/*++

Routine Description:

    This routine re-acquires references to the metadata file on the specified instance.

Arguments:

    Cbd                 - Supplies a pointer to the callbackData which
                          declares the requested operation.

Return Value:

    Status

Note:

    This routine takes care of the synchronization needed to access the metadata
    file object and handle


    This routine will also NULL the MetadataOpenTriggerFileObject in the instance context
    if it was successfully able to open the metadata file references.


--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    PFMM_INSTANCE_CONTEXT instanceContext = NULL;

    PAGED_CODE();

    //
    //  Get the instance context
    //

    status = FltGetInstanceContext( Cbd->Iopb->TargetInstance,
                                    &instanceContext );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmReacquireMetadataFileReferences -> Failed to get instance context.\n") );

        goto FmmReacquireMetadataFileReferencesCleanup;
    }

    //
    //  Acquire exclusive access to the instance context
    //

    FmmAcquireResourceExclusive( &instanceContext->MetadataResource );

    if (FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION)) {

        //
        //  If this instance context is in a transition state, it implies that
        //  the instance context lock has been released while sending an operation
        //  down to the file system. The reason for doing so is to prevent a potential
        //  deadlock if an underlying filter sends an IO to the top of the filter
        //  stack while we are holding the resource
        //
        //  We have managed to acquire this resource in this state of transition.
        //  It would be incorrect to use or modify the instance context in any way
        //  in this situation. So we simply let go.
        //

        status = STATUS_FILE_LOCK_CONFLICT;

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmReacquireMetadataFileReferences -> Failed to get exclusive access to instance context since it is in a state of transition.\n") );
    } else {

        //
        //  Re-open the metadata only if the trigger file object match the file object that
        //  caused this function to be called
        //

        if (instanceContext->MetadataOpenTriggerFileObject == Cbd->Iopb->TargetFileObject) {

            //
            //  Open the filter metadata file (do not read the file since we already have
            //  stuff in memory and do not create if the file does not exist
            //

            if (!FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED )) {

                DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                            ("[Fmm]: FmmReacquireMetadataFileReferences -> Re-acquiring references to metadata handle and file object (InstanceContext = %p, VolumeFileObject = %p)\n",
                             instanceContext,
                             Cbd->Iopb->TargetFileObject) );

                status = FmmOpenMetadata( instanceContext,
                                                 FALSE );

                //
                //  Reset the trigger file object since the volume open failed.
                //

                instanceContext->MetadataOpenTriggerFileObject = NULL;

            } else {

                DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                            ("[Fmm]: FmmReacquireMetadataFileReferences -> Exit without attempting to re-acquire references to metadata handle and file object (InstanceContext = %p, VolumeFileObject = %p, MetadataOpenTriggerFileObject = %p, MetadataAlreadyOpen = 0x%x)\n",
                             instanceContext,
                             Cbd->Iopb->TargetFileObject,
                             instanceContext->MetadataOpenTriggerFileObject,
                             FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED )) );
            }
        } else {


            DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                        ("[Fmm]: FmmReacquireMetadataFileReferences -> Exit without attempting to re-acquire references to metadata handle and file object (InstanceContext = %p, VolumeFileObject = %p, MetadataOpenTriggerFileObject = %p, MetadataAlreadyOpen = 0x%x)\n",
                         instanceContext,
                         Cbd->Iopb->TargetFileObject,
                         instanceContext->MetadataOpenTriggerFileObject,
                         FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED )) );
        }
    }

    //
    //  Relinquish exclusive access to the instance context
    //

    FmmReleaseResource( &instanceContext->MetadataResource );


FmmReacquireMetadataFileReferencesCleanup:

    //
    // Release the references we have acquired
    //

    if (instanceContext != NULL) {

        FltReleaseContext( instanceContext );
    }


    return status;;

}



NTSTATUS
FmmSetMetadataOpenTriggerFileObject (
    _Inout_ PFLT_CALLBACK_DATA Cbd
    )
/*++

Routine Description:

    This routine sets the MetadataOpenTriggerFileObject in the instance context
    to the file object of the volume that triggered the release of the metadata file
    references.

Arguments:

    Cbd                 - Supplies a pointer to the callbackData which
                          declares the requested operation.

Return Value:

    Status

Note:

    This routine takes care of the synchronization needed to access the metadata
    file object and handle


--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PFMM_INSTANCE_CONTEXT instanceContext = NULL;

    PAGED_CODE();

    //
    //  Get the instance context
    //

    status = FltGetInstanceContext( Cbd->Iopb->TargetInstance,
                                    &instanceContext );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmSetMetadataOpenTriggerFileObject -> Failed to get instance context.\n") );

        goto FmmSetMetadataOpenTriggerFileObjectCleanup;
    }

    //
    //  Acquire exclusive access to the instance context
    //

    FmmAcquireResourceExclusive( &instanceContext->MetadataResource );

    if (FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION)) {

        //
        //  If this instance context is in a transition state, it implies that
        //  the instance context lock has been released while sending an operation
        //  down to the file system. The reason for doing so is to prevent a potential
        //  deadlock if an underlying filter sends an IO to the top of the filter
        //  stack while we are holding the resource
        //
        //  We have managed to acquire this resource in this state of transition.
        //  It would be incorrect to use or modify the instance context in any way
        //  in this situation. So we simply let go.
        //

        status = STATUS_FILE_LOCK_CONFLICT;

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmSetMetadataOpenTriggerFileObject -> Failed to get exclusive access to instance context since it is in a state of transition.\n") );
    } else {

        DebugTrace( DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmSetMetadataOpenTriggerFileObject -> Setting MetadataOpenTriggerFileObject to %p (OldValue = %p).\n",
                     Cbd->Iopb->TargetFileObject,
                     instanceContext->MetadataOpenTriggerFileObject) );


        //
        //  Save the volume file object as the trigger file object
        //

        FLT_ASSERT((instanceContext->MetadataOpenTriggerFileObject == NULL) ||
                   (instanceContext->MetadataOpenTriggerFileObject == Cbd->Iopb->TargetFileObject));

        instanceContext->MetadataOpenTriggerFileObject = Cbd->Iopb->TargetFileObject;
    }

    //
    //  Relinquish exclusive access to the instance context
    //

    FmmReleaseResource( &instanceContext->MetadataResource );


FmmSetMetadataOpenTriggerFileObjectCleanup:

    //
    // Release the references we have acquired
    //

    if (instanceContext != NULL) {

        FltReleaseContext( instanceContext );
    }


    return status;;
}

_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(InstanceContext->MetadataResource)
_Releases_lock_(InstanceContext->MetadataResource)
_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
VOID
FmmBeginFileSystemOperation (
    IN PFMM_INSTANCE_CONTEXT InstanceContext
    )
/*++

Routine Description:

    This routine must be called before the filter performs a file system operation
    if it is holding an exclusive lock to the instance context resource at the
    time it needs to perform the file system operation

Arguments:

    InstanceContext     - Supplies the instance context for this instance.

Return Value:

    Returns the status of this operation.

Note:

    The caller must hold the instance context resource exclusive when this routine is called.

--*/
{
    PAGED_CODE();

    //
    //  Release the instance context lock before sending an operation down to the
    //  file system. The reason for doing so is to prevent a potential deadlock if
    //  an underlying filter sends an IO to the top of the filter stack while we
    //  are holding the resource
    //
    //  Before we release the lock we mark the instance context to indicate it is
    //  in a transition state. Any other thread that finds the instance context in a
    //  transition state will not use or modify the  instance context
    //
    //  This thread can however continue to use/modify the instance context since it
    //  is guaranteed exclusive access. Other threads that see the instance context
    //  in a transition state will not use or modify the context
    //

    FLT_ASSERT( !FlagOn( InstanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION ) );

    SetFlag( InstanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION );

    //
    //  Relinquish exclusive access to the instance context
    //

    FmmReleaseResource( &InstanceContext->MetadataResource );

}


_Acquires_lock_(_Global_critical_region_)
_Requires_lock_not_held_(InstanceContext->MetadataResource)    
_Acquires_exclusive_lock_(InstanceContext->MetadataResource)
_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
VOID
FmmEndFileSystemOperation (
    IN PFMM_INSTANCE_CONTEXT InstanceContext
    )
/*++

Routine Description:

    This routine must be called after the filter performs a file system operation
    if it was holding an exclusive lock to the instance context resource at the
    time it needed to perform the file system operation

Arguments:

    InstanceContext     - Supplies the instance context for this instance.

Return Value:

    Returns the status of this operation.

Note:

    The caller will hold the instance context resource exclusive when this routine returns.

--*/
{
    PAGED_CODE();

    //
    //  Acquire exclusive access to the instance context
    //

    FmmAcquireResourceExclusive( &InstanceContext->MetadataResource );


    //
    //  Sanity - nothing should have changed this flag while we dropped the resource
    //  because all other threads will not use or modify the instance context while
    //  this flag is set
    //

    FLT_ASSERT( FlagOn( InstanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION ) );


    //
    //  Reset the flag to indicate that the instance context is no longer in
    //  a transition state
    //

    ClearFlag( InstanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION );

}



#if VERIFY_METADATA_OPENED

NTSTATUS
FmmIsMetadataOpen (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _Out_ BOOLEAN* MetadataOpen
    )
/*++

Routine Description:

    This routine returns if the metadata file is open on the specified instance.

Arguments:

    Cbd                 - Supplies a pointer to the callbackData which
                          declares the requested operation.
    MetadataOpen        - Returns if the metadata file is open

Return Value:

    Status

Note:

    This routine takes care of the synchronization needed to access the metadata
    file object and handle


--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PFMM_INSTANCE_CONTEXT instanceContext = NULL;


    //
    //  Get the instance context
    //

    status = FltGetInstanceContext( Cbd->Iopb->TargetInstance,
                                    &instanceContext );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmIsMetadataOpen -> Failed to get instance context.\n") );

        goto FmmIsMetadataOpenCleanup;
    }

    //
    //  Acquire exclusive access to the instance context
    //

    FmmAcquireResourceShared( &instanceContext->MetadataResource );

    if (FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION)) {

        //
        //  If this instance context is in a transition state, it implies that
        //  the instance context lock has been released while sending an operation
        //  down to the file system. The reason for doing so is to prevent a potential
        //  deadlock if an underlying filter sends an IO to the top of the filter
        //  stack while we are holding the resource
        //
        //  We have managed to acquire this resource in this state of transition.
        //  It would be incorrect to use or modify the instance context in any way
        //  in this situation. So we simply let go.
        //

        status = STATUS_FILE_LOCK_CONFLICT;

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                    ("[Fmm]: FmmIsMetadataOpen -> Failed to get exclusive access to instance context since it is in a state of transition.\n") );
    } else {

        //
        //  Return if the metadata is opened
        //

        *MetadataOpen = BooleanFlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED );

        //
        //  Sanity - verify that this flag is reflecting the correct state of the metadata file
        //

        FLT_ASSERT ( (FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED ) &&
                      (instanceContext->MetadataFileObject != NULL) &&
                      (instanceContext->MetadataHandle != NULL)) ||
                     (!FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED ) &&
                      (instanceContext->MetadataFileObject == NULL) &&
                      (instanceContext->MetadataHandle == NULL)) );

    }

    //
    //  Relinquish exclusive access to the instance context
    //

    FmmReleaseResource( &instanceContext->MetadataResource );


FmmIsMetadataOpenCleanup:

    //
    // Release the references we have acquired
    //

    if (instanceContext != NULL) {

        FltReleaseContext( instanceContext );
    }


    return status;
}


#endif


