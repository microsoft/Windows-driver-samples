/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    operations.c

Abstract:

    This is the i/o operations module of the kernel mode filter driver implementing
    filter metadata management.


Environment:

    Kernel mode


--*/

#include "pch.h"

//
//  Missing error code on Win2k
//

#if (WINVER==0x0500)
#ifndef STATUS_INVALID_DEVICE_OBJECT_PARAMETER
#define STATUS_INVALID_DEVICE_OBJECT_PARAMETER ((NTSTATUS)0xC0000369L)
#endif
#endif

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FmmPreCreate)
#pragma alloc_text(PAGE, FmmPostCreate)
#pragma alloc_text(PAGE, FmmPreCleanup)
#pragma alloc_text(PAGE, FmmPostCleanup)
#pragma alloc_text(PAGE, FmmPreFSControl)
#pragma alloc_text(PAGE, FmmPostFSControl)
#pragma alloc_text(PAGE, FmmPreDeviceControl)
#pragma alloc_text(NONPAGED, FmmPostDeviceControl)
#pragma alloc_text(PAGE, FmmPreShutdown)
#pragma alloc_text(PAGE, FmmPrePnp)
#pragma alloc_text(PAGE, FmmPostPnp)

#endif

FLT_PREOP_CALLBACK_STATUS
FmmPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    NTSTATUS status;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;
    BOOLEAN isImpliedLock = FALSE;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreCreate -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );


    //
    // Initialize defaults
    //

    status = STATUS_SUCCESS;
    callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK; // pass through - default is no post op callback

     //
     //  Sanity check to ensure that the volume detection logic works
     //
     //  If the filename length is 0 and the related file object is NULL,
     //  the the FO_VOLUME_OPEN flag must be set
     //

     FLT_ASSERT( (!(Cbd->Iopb->TargetFileObject->FileName.Length == 0 &&
                    Cbd->Iopb->TargetFileObject->RelatedFileObject == NULL)) ||
                  FlagOn( Cbd->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN ) );

    if (FmmTargetIsVolumeOpen( Cbd )) {

        //
        //  Check for implicit volume locks (primarily used by autochk)
        //

        status = FmmIsImplicitVolumeLock( Cbd, &isImpliedLock );

        FLT_ASSERT( NT_SUCCESS( status ) );

        if (isImpliedLock) {

            //
            //  This is an implicit volume lock
            //

            //
            //  Give up the metadata file handle and the metadata file object
            //

            status = FmmReleaseMetadataFileReferences( Cbd );

            if ( NT_SUCCESS( status )) {

                //
                //  Continue with the lock/dismount - we need to check if the
                //  lock operation suceeded in the post-op
                //

                callbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
            } else {

                //
                //  Fail the lock/dismount
                //

                DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                            ("[Fmm]: Failed to release metadata file references with status 0x%x for a volume lock/dismount\n",
                             status) );

                //
                //  Since this operation has failed, FmmPreCreateCleanup will
                //  update Cbd->IoStatus.Status with the status code and
                //  complete the operation by returning FLT_PREOP_COMPLETE
                //

            }

        }

        //
        //  We do not need to process volume opens any further
        //

         goto FmmPreCreateCleanup;
    }


#if VERIFY_METADATA_OPENED

    //
    //  For all non-volume opens, check if the metadata is open in the post-op
    //
          
    callbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

#endif


    //
    //  Here the filter can do any further processing it may want to do
    //  in the PreCreate Callback
    //



FmmPreCreateCleanup:


    //
    // If any operation has failed then complete and fail the call
    //

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Fmm]: FmmPreCreate -> Failed with status 0x%x \n",
                    status) );

        Cbd->IoStatus.Status = status;
        callbackStatus = FLT_PREOP_COMPLETE;
    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreCreate -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    return callbackStatus;

}


FLT_POSTOP_CALLBACK_STATUS
FmmPostCreate (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    NTSTATUS status;
    BOOLEAN isImpliedLock = FALSE;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CbdContext );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPostCreate -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    // Initialize defaults
    //

    status = STATUS_SUCCESS;

    if (!FlagOn(Flags,FLTFL_POST_OPERATION_DRAINING) &&
        FmmTargetIsVolumeOpen( Cbd )) {

        //
        //  Check for implicit volume locks (primarily used by autochk)
        //

        status = FmmIsImplicitVolumeLock( Cbd, &isImpliedLock );

        FLT_ASSERT( NT_SUCCESS( status ) );

        if (isImpliedLock) {

            //
            //  This is an implicit volume lock
            //

            if (!NT_SUCCESS( Cbd->IoStatus.Status )) {

                //
                //  The lock failed - reaquire our references to the metadata file
                //  handle and the metadata file object
                //

                status = FmmReacquireMetadataFileReferences( Cbd );

                if (!NT_SUCCESS( status )) {

                    DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                                ("[Fmm]: Failed to re-open metadata with status 0x%x after a failed lock with status 0x%x\n",
                                 status,
                                 Cbd->IoStatus.Status) );

                    //
                    //  Sanity - we are now in a bad state. The lock has failed
                    //  but we have not been able to re-acquire references to
                    //  our metadata file
                    //
                    //  It is always possible to fail with STATUS_INSUFFICIENT_RESOURCES
                    //  so we should ignore that.
                    //
                    //  It is also possible to fail if the instance context was in a transition state
                    //  so we should ignore STATUS_FILE_LOCK_CONFLICT too.
                    //

                    FLT_ASSERT( (status == STATUS_INSUFFICIENT_RESOURCES) ||
                                (status == STATUS_FILE_LOCK_CONFLICT) );
                }

            } else {

                //
                //  The lock operation suceeded - update the
                //  MetadataOpenTriggerFileObject in the instance context to
                //  the File Object that performed the lock operation.  This
                //  is so we can recognize an implicit unlock at close time.
                //
                //  You may have noticed that we set the
                //  MetadataOpenTriggerFileObject in pre-create and may be
                //  wondering why we set it again in this case.  This is to
                //  support a lower filter doing a recursive lock operation
                //  from the top of the stack.
                //

                status = FmmSetMetadataOpenTriggerFileObject( Cbd );

                if (!NT_SUCCESS( status )) {

                    DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                                ("[Fmm]: Failed to update MetadataOpenTriggerFileObject in the instance context with status 0x%x after a successful lock.\n",
                                 status) );

                    //
                    //  Sanity - we are now in a bad state. We have failed to set the TriggerFileObject
                    //  We may not be able to detect an unlock operation on which we need to
                    //  re-acquire our metadata file references
                    //

                    FLT_ASSERT( status == STATUS_FILE_LOCK_CONFLICT );

                }
            }
        }

        //
        //  We do not need to process volume opens any further
        //

        goto FmmPostCreateCleanup;
    }

#if VERIFY_METADATA_OPENED

    //
    //  For all successful non-volume opens, check if the metadata is open 
    //
    
    if (NT_SUCCESS( Cbd->IoStatus.Status )) {

        BOOLEAN metadataOpen;
        
        status = FmmIsMetadataOpen( Cbd, &metadataOpen );

        if (NT_SUCCESS( status) && !metadataOpen) { 

            DebugTrace( DEBUG_TRACE_ERROR,
                        ("[Fmm]: FmmPostCreate -> Create successful but metadata not open \n") );
        }

        FLT_ASSERT( ((NT_SUCCESS( status) && metadataOpen) || 
                     (status == STATUS_FILE_LOCK_CONFLICT)) ); 
    }    

#endif

    //
    //  Here the filter can do any further processing it may want to do
    //  in the PostCreate Callback
    //

FmmPostCreateCleanup:


    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Fmm]: FmmPostCreate -> Failed with status 0x%x \n",
                    status) );

        //
        //  It does not make sense to fail in the the post op, since the operation has completed
        //
    }


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPostCreate -> Exit (Cbd = %p, FileObject = %p, Status = 0x%08X)\n",
                 Cbd,
                 FltObjects->FileObject,
                 Cbd->IoStatus.Status) );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
FmmPreCleanup (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{

    UNREFERENCED_PARAMETER( Cbd );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreCleanup -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreCleanup -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    return FLT_PREOP_SYNCHRONIZE;
}


FLT_POSTOP_CALLBACK_STATUS
FmmPostCleanup (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    NTSTATUS status;


    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    //
    //  The pre-operation callback will return FLT_PREOP_SYNCHRONIZE if it needs a
    //  post operation callback. In this case, the Filter Manager will call the
    //  minifilter's post-operation callback in the context of the pre-operation
    //  thread, at IRQL <= APC_LEVEL. This allows the post-operation code to be
    //  pagable and also allows it to access paged data
    //

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPostCleanup -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    // Initialize defaults
    //

    status = STATUS_SUCCESS;


    if (!FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING ) &&
        FmmTargetIsVolumeOpen( Cbd )) {

        if (NT_SUCCESS( Cbd->IoStatus.Status )) {

            //
            //  A close on a volume handle could be an unlock if a lock was
            //  previously  called on this handle. Check if this was a close
            //  on a volume handle on which a lock was previously successful.
            //  If so, re-acquire the references to our metadata file
            //

            status = FmmReacquireMetadataFileReferences( Cbd );

            if (!NT_SUCCESS( status )) {

                DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                            ("[Fmm]: Failed to re-open metadata with status 0x%x after a successful unlock.\n",
                             status) );

                //
                //  Sanity - we are now in a bad state. The volume was unlocked
                //  but we have not been able to re-acquire references to
                //  our metadata file
                //
                //  Ntfs dismounts and remounts the volume after an unlock. So we ignore
                //  failures to open the metadata with STATUS_INVALID_DEVICE_OBJECT_PARAMETER
                //  or STATUS_FILE_INVALID because the volume should have been remounted and 
                //  the metadata file should have been opened on the newly mounted instance 
                //  of that volume
                //
                //  Note however, that if this is an implicit lock (used by autoXXX.exe) then
                //  ntfs will not automatically dismount the volume. It relies on the application
                //  to restart the system if it has made any changes to the volume. If the
                //  application has not made any changes then ntfs will simply continue on
                //  after the unlock without dismounting the volume. Hence we cannot assume
                //  that ntfs always dismounts the volume. We need to try to re-acquire
                //  a handle to our metadata file and ignore failure with error
                //  STATUS_INVALID_DEVICE_OBJECT_PARAMETER or STATUS_NO_MEDIA_IN_DEVICE which 
                //  indicate that the volume has been dismounted
                //
                //  Also it is always possible to fail with STATUS_INSUFFICIENT_RESOURCES
                //  so we should ignore that as well.
                //
                //  It is also possible to fail if the instance context was in a transition state
                //  so we should ignore STATUS_FILE_LOCK_CONFLICT too.
                //

                FLT_ASSERT( (status == STATUS_INVALID_DEVICE_OBJECT_PARAMETER) ||
                            (status == STATUS_NO_MEDIA_IN_DEVICE) ||
                            (status == STATUS_INSUFFICIENT_RESOURCES) ||
                            (status == STATUS_FILE_LOCK_CONFLICT) ||
                            (status == STATUS_FILE_INVALID) );

                //
                //  There is little use updating the return status since it already has a
                //  failure code from the failed dismount
                //
            }
        }

        //
        //  We don't need to process a volume CleanUp any further
        //

        goto FmmPostCleanupCleanup;
    }


    //
    //  Here the filter can do any further processing it may want to do
    //  in the PostCleanUp Callback
    //

FmmPostCleanupCleanup:

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Fmm]: FmmPostCleanup -> Failed with status 0x%x \n",
                    status) );

        //
        //  It does not make sense to fail in the the post op, since the operation has completed
        //

    }


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPostCleanup -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
FmmPreFSControl (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{

    NTSTATUS status;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreFsCtl -> Enter (FsControlCode = 0x%x, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->Parameters.FileSystemControl.Common.FsControlCode,
                 Cbd,
                 FltObjects->FileObject) );


    //
    //  default to no post-op callback
    //

    callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;


    if (Cbd->Iopb->MinorFunction != IRP_MN_USER_FS_REQUEST) {

        goto FmmPreFSControlCleanup;
    }


    switch (Cbd->Iopb->Parameters.FileSystemControl.Common.FsControlCode) {

    //
    //  System FSCTLs that we are interested in
    //

    case FSCTL_DISMOUNT_VOLUME:
    case FSCTL_LOCK_VOLUME:


        if (FmmTargetIsVolumeOpen( Cbd )) {

            //
            //  Give up the metadata file handle and the metadata file object
            //

            status = FmmReleaseMetadataFileReferences( Cbd );

            if ( NT_SUCCESS( status )) {

                //
                //  Continue with the lock/dismount - we need to check if the
                //  lock operation suceeded in the post-op
                //

                callbackStatus = FLT_PREOP_SYNCHRONIZE;
            } else {

                //
                //  Fail the lock/dismount
                //

                DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                            ("[Fmm]: Failed to release metadata file references with status 0x%x for a volume lock/dismount\n",
                             status) );

                Cbd->IoStatus.Status = status;
                callbackStatus = FLT_PREOP_COMPLETE;
            }
        }
        break;

    case FSCTL_UNLOCK_VOLUME:

        //
        //  We need to handle unlock in the post-op
        //

        callbackStatus = FLT_PREOP_SYNCHRONIZE;
        break;

    }

FmmPreFSControlCleanup:


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreFsCtl -> Exit (FsControlCode = 0x%x, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->Parameters.FileSystemControl.Common.FsControlCode,
                 Cbd,
                 FltObjects->FileObject) );

    return callbackStatus;

}


FLT_POSTOP_CALLBACK_STATUS
FmmPostFSControl (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{

    NTSTATUS status;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPostFsCtl -> Enter (FsControlCode = 0x%x, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->Parameters.FileSystemControl.Common.FsControlCode,
                 Cbd,
                 FltObjects->FileObject) );


    if (!FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING )) {

        switch (Cbd->Iopb->Parameters.FileSystemControl.Common.FsControlCode) {

        //
        //  System FSCTLs that we are interested in
        //

        case FSCTL_DISMOUNT_VOLUME:

            if (FmmTargetIsVolumeOpen( Cbd )) {

                if (NT_SUCCESS( Cbd->IoStatus.Status )) {

                    //
                    //  Dismount succeeded - teardown our instance because its no longer valid.
                    //  If we do not tear down this instance, it will stay around until the
                    //  last handle for that volume is closed. This will cause the instance
                    //  enumeration APIs to see multiple instances
                    //

                    status = FltDetachVolume( Globals.Filter, FltObjects->Volume, NULL );

                    if (!NT_SUCCESS( status )) {

                        DebugTrace( DEBUG_TRACE_ERROR,
                                    ("[Fmm]: Failed to detach instance with status 0x%x after a volume dismount\n",
                                     status) );

                        //
                        //  Doesn't make sense to update the status code in the post-op with a
                        //  failure code since the operation has already been performed by the
                        //  file system
                        //
                    }

                } else {

                    //
                    //  The dismount failed - reaquire our references to the metadata file
                    //  handle and the metadata file object
                    //

                    status = FmmReacquireMetadataFileReferences( Cbd );

                    if (!NT_SUCCESS( status )) {

                        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                                    ("[Fmm]: Failed to re-open metadata with status 0x%x after a failed dismount with status 0x%x\n",
                                     status,
                                     Cbd->IoStatus.Status) );

                        //
                        //  Sanity - we are now in a bad state. The dismount has failed
                        //  but we have not been able to re-acquire references to
                        //  our metadata file
                        //
                        //  It is always possible to fail with STATUS_INSUFFICIENT_RESOURCES
                        //  so we should ignore that.
                        //
                        //  It is also possible to fail if the instance context was in a transition state
                        //  so we should ignore STATUS_FILE_LOCK_CONFLICT too.
                        //

                        FLT_ASSERT( (status == STATUS_INSUFFICIENT_RESOURCES) ||
                                    (status == STATUS_FILE_LOCK_CONFLICT) );

                        //
                        //  There is little use updating the return status since it already has a
                        //  failure code from the failed dismount
                        //
                    }
                }
            }

            break;

        case FSCTL_LOCK_VOLUME:


            if (FmmTargetIsVolumeOpen( Cbd )) {

                if (!NT_SUCCESS( Cbd->IoStatus.Status )) {

                    //
                    //  The lock failed - reaquired our references to the metadata file
                    //  handle and the metadata file object
                    //

                    status = FmmReacquireMetadataFileReferences( Cbd );

                    if (!NT_SUCCESS( status )) {

                        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                                    ("[Fmm]: Failed to re-open metadata with status 0x%x after a failed lock with status 0x%x\n",
                                     status,
                                     Cbd->IoStatus.Status) );

                        //
                        //  Sanity - we are now in a bad state. The lock has failed
                        //  but we have not been able to re-acquire references to
                        //  our metadata file
                        //
                        //  It is always possible to fail with STATUS_INSUFFICIENT_RESOURCES
                        //  so we should ignore that.
                        //
                        //  It is also possible to fail if the instance context was in a transition state
                        //  so we should ignore STATUS_FILE_LOCK_CONFLICT too.
                        //

                        FLT_ASSERT( (status == STATUS_INSUFFICIENT_RESOURCES) ||
                                    (status == STATUS_FILE_LOCK_CONFLICT) );

                        //
                        //  There is little use updating the return status since it already has a
                        //  failure code from the failed lock
                        //

                    }
                } else {

                    //
                    //  The lock operation suceeded - update the MetadataOpenTriggerFileObject in the
                    //  instance context to the File Object on the lock operation suceeded because this
                    //  the file object on close/unlock of which, we need to reacquire our metadata file
                    //  references
                    //

                    status = FmmSetMetadataOpenTriggerFileObject( Cbd );

                    if (!NT_SUCCESS( status )) {

                        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                                    ("[Fmm]: Failed to update MetadataOpenTriggerFileObject in the instance context with status 0x%x after a successful lock.\n",
                                     status) );

                        //
                        //  Sanity - we are now in a bad state. We have failed to
                        //  set the TriggerFileObject We may not be able to detect
                        //  an unlock operation on which we need to re-acquire our
                        //  metadata file references
                        //

                        FLT_ASSERT( status == STATUS_FILE_LOCK_CONFLICT );

                        //
                        //  Doesn't make sense to update the status code in the
                        //  post-op with a failure code since the operation has
                        //  already been performed by the file system
                        //
                    }

                }
            }

            break;

        case FSCTL_UNLOCK_VOLUME:

            if (NT_SUCCESS( Cbd->IoStatus.Status )) {

                //
                //  The unlock suceeded - reaquired our references to the metadata file
                //  handle and the metadata file object
                //

                status = FmmReacquireMetadataFileReferences( Cbd );

                if (!NT_SUCCESS( status )) {

                    DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                                ("[Fmm]: Failed to re-open metadata with status 0x%x after a successful unlock.\n",
                                 status) );

                    //
                    //  Sanity - we are now in a bad state. The volume was unlocked
                    //  but we have not been able to re-acquire references to
                    //  our metadata file
                    //
                    //  Ntfs dismounts and remounts the volume after an unlock. So we ignore
                    //  failures to open the metadata with STATUS_INVALID_DEVICE_OBJECT_PARAMETER
                    //  because the volume should have been remounted and the metadata file
                    //  should have been opened on the newly mounted instance of that volume
                    //
                    //  Note however, that if this is an implicit lock (used by autoXXX.exe) then
                    //  ntfs will not automatically dismount the volume. It relies on the application
                    //  to restart the system if it has made any changes to the volume. If the
                    //  application has not made any changes then ntfs will simply continue on
                    //  after the unlock without dismounting the volume. Hence we cannot assume
                    //  that ntfs always dismounts the volume. We need to try to re-acquire
                    //  a handle to our metadata file and ignore failure with error
                    //  STATUS_INVALID_DEVICE_OBJECT_PARAMETER which indicates that the
                    //  volume has dismounted
                    //
                    //  Also it is always possible to fail with STATUS_INSUFFICIENT_RESOURCES
                    //  so we should ignore that as well.
                    //
                    //  It is also possible to fail if the instance context was in a transition state
                    //  so we should ignore STATUS_FILE_LOCK_CONFLICT too.
                    //

                    FLT_ASSERT( (status == STATUS_INVALID_DEVICE_OBJECT_PARAMETER) ||
                                (status == STATUS_INSUFFICIENT_RESOURCES) ||
                                (status == STATUS_FILE_LOCK_CONFLICT) );

                    //
                    //  There is little use updating the return status since it already has a
                    //  failure code from the failed dismount
                    //
                }
            }


            break;

        }
    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPostFsCtl -> Exit (FsControlCode = 0x%x, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->Parameters.FileSystemControl.Common.FsControlCode,
                 Cbd,
                 FltObjects->FileObject) );


    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
FmmPreDeviceControl (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{

    NTSTATUS status = STATUS_SUCCESS;
    FLT_PREOP_CALLBACK_STATUS callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PFMM_INSTANCE_CONTEXT instanceContext = NULL;

    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    *CompletionContext = NULL;


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreDeviceControl -> Enter (IoControlCode = 0x%x, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->Parameters.DeviceIoControl.Common.IoControlCode,
                 Cbd,
                 FltObjects->FileObject) );


    switch (Cbd->Iopb->Parameters.DeviceIoControl.Common.IoControlCode) {

    //
    //  System IOCTLs that we are interested in
    //

    case IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES:

        //
        //  We want the snapshot to have a consistent image
        //  of our metadata file that is in sync with the state
        //  of the volume
        //
    
        //
        //  Get the instance context
        //

        status = FltGetInstanceContext( Cbd->Iopb->TargetInstance,
                                        &instanceContext );
        if (!NT_SUCCESS( status )) {

            DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                        ("[Fmm]: Failed to get instance context in FmmPreDeviceControl.\n") );

            goto FmmPreDeviceControlCleanup;
        }


        //
        //  Here the filter must flush any portion of its metadata
        //  that it has not flushed to disk
        //
        //  If the filter is using mapped cache buffers to read/write
        //  its metadata then the File System will take care of all the
        //  flushing. The filter just needs to ensure that it does not
        //  write to any of its mapped cache buffers while the FS is
        //  trying to flush changes out to disk.
        //

        //
        //  After this point, the filter should not be sending any updates 
        //  to its metadata file on disk until the post-op callback for 
        //  IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES 
        //
        //  The filter would do this by marking its instance context in some way
        //  (say, by setting a flag) to indicate to other threads that they should 
        //  not try to update the metadata file on the disk
        // 

        //
        //  Do not release the instance context but instead pass it to the PostOp
        //  The PostOp routine would need to unmark the instance context in some way
        //  to indicate that it is now ok to update the metadata file on the disk.
        //
        //  Since we do not want to fail this unmarking because we cannot acquire the
        //  instance context in the post-op, it is better to pass the instance context from
        //  PreOp to PostOp
        //

        *CompletionContext = instanceContext;

        //
        //  Force a post-op so we may undo our marking and release the instance context 
        //

        callbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

        break;

    }

FmmPreDeviceControlCleanup:


    //
    // If any operation has failed then complete and fail the call
    //

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Fmm]: FmmPreDeviceControl -> Failed with status 0x%x \n",
                    status) );

        //
        //  We are not having a post-op since the pre-op failed
        //  Release the instance context
        //

        if (instanceContext != NULL) {

            FltReleaseContext( instanceContext );
            *CompletionContext = NULL;
        }

        Cbd->IoStatus.Status = status;
        callbackStatus = FLT_PREOP_COMPLETE;
    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreDeviceControl -> Exit (IoControlCode = 0x%x, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->Parameters.DeviceIoControl.Common.IoControlCode,
                 Cbd,
                 FltObjects->FileObject) );

    return callbackStatus;

}


FLT_POSTOP_CALLBACK_STATUS
FmmPostDeviceControl (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{

    PFMM_INSTANCE_CONTEXT instanceContext = NULL;

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( FltObjects );


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPostDeviceControl -> Enter (IoControlCode = 0x%x, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->Parameters.DeviceIoControl.Common.IoControlCode,
                 Cbd,
                 FltObjects->FileObject) );

    //
    //  We need to do this even if we are draining
    //

    switch (Cbd->Iopb->Parameters.DeviceIoControl.Common.IoControlCode) {

    //
    //  System IOCTLs that we are interested in
    //

    case IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES:

        //
        //  Assign the instance context
        //

        instanceContext = (PFMM_INSTANCE_CONTEXT) CbdContext;

        //
        //  Sanity
        //

        FLT_ASSERT( instanceContext != NULL );


        //
        //  At this point, it is ok for the filter to send updates to its metadata
        //  file on disk
        //
        //  The filter would do this by unmarking its instance context (say by,
        //  resetting the flag that it set in the PreOp) to indicate to other threads 
        //  that it is now ok to update the metadata file on the disk
        // 


        //
        //  Release the instance context
        //

        FltReleaseContext( instanceContext );

        break;

    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPostDeviceControl -> Exit (IoControlCode = 0x%x, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->Parameters.DeviceIoControl.Common.IoControlCode,
                 Cbd,
                 FltObjects->FileObject) );


    return FLT_POSTOP_FINISHED_PROCESSING;
}



FLT_PREOP_CALLBACK_STATUS
FmmPreShutdown (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Cbd );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreShutdown -> Enter (Cbd = %p, FileObject = %p, Volume = %p)\n",
                 Cbd,
                 FltObjects->FileObject,
                 FltObjects->Volume) );

    status = FltDetachVolume( Globals.Filter, FltObjects->Volume, NULL );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Fmm]: Failed to detach instance with status 0x%x on system shutdown\n",
                     status) );

        //
        //  Doesn't really make sense to fail a shutdown, even if this operation failed
        //

    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPreShutdown -> Exit (Cbd = %p, FileObject = %p, Volume = %p)\n",
                 Cbd,
                 FltObjects->FileObject,
                 FltObjects->Volume) );

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_PREOP_CALLBACK_STATUS
FmmPrePnp (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*

Routine Description:

    This routine handles the pre-processing of all PNP operations received
    on this instance. It handles query, cancel and suprise device removal.
    For query device removal we have to close all open file handles we hold
    so that the base file system can correctly response to these PNP requests.

Arguments:

    Cbd - Pointer to the FLT_CALLBACK_DATA structure containing all the relevant
        parameters for this operation.

    FltObject - Pointer to the FLT_RELATED_OBJECTS data structure containing,
        opaque handles to this filter, instance and its associated volume.

    CompletionContext - Not used.

Return Value:

    FLT_PREOP_SUCCESS_NO_CALLBACK as we are done with our processing and are
    not interested in a post-operartion callback.

*/
{
    NTSTATUS status;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;

    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPrePnp -> Enter (Cbd = %p, FileObject = %p, Volume = %p)\n",
                 Cbd,
                 FltObjects->FileObject,
                 FltObjects->Volume) );

    //
    //  default to no post op callback
    //

    callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    switch (Cbd->Iopb->MinorFunction) {

    case IRP_MN_QUERY_REMOVE_DEVICE:

        //
        //  Give up the metadata file handle and the metadata file object
        //

        status = FmmReleaseMetadataFileReferences( Cbd );

        if (!NT_SUCCESS( status )) {

            //
            // Fail the query removal
            //

            DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                        ("[Fmm]: Failed to release metadata file references with status 0x%x for query removal\n",
                         status) );

            Cbd->IoStatus.Status = status;
            callbackStatus = FLT_PREOP_COMPLETE;
        }

        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        //
        //  We need to pass this notification through to the file system
        //  so he start allowing IO to the volume. We file for a post op
        //  so that we can reacquire our resources. We must return 
        //  FLT_PREOP_SYNCRONIZE because we need to be below DPC inorder
        //  to reopen our metadata file.
        //

        callbackStatus = FLT_PREOP_SYNCHRONIZE;

        break;

    case IRP_MN_SURPRISE_REMOVAL:

        //
        //  Teardown our instance because its no longer valid.
        //

        status = FltDetachVolume( Globals.Filter, FltObjects->Volume, NULL );

        if (!NT_SUCCESS( status )) {

            DebugTrace( DEBUG_TRACE_ERROR,
                        ("[Fmm]: Failed to detach instance with status 0x%x after a surprise removal\n",
                         status) );

        }

        break;

    default:

        //
        //  Pass all PNP minor codes we don't care about.
        //

        break;
    }


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Fmm]: FmmPrePnp -> Exit (Cbd = %p, FileObject = %p, Volume = %p)\n",
                 Cbd,
                 FltObjects->FileObject,
                 FltObjects->Volume) );

    return callbackStatus;
}

FLT_POSTOP_CALLBACK_STATUS
FmmPostPnp (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*

Routine Description:
    
    This routine handles the post-processing of IRP_MN_CANCEL_REMOVE_DEVICE.
    We reacquire our references to the metadata file handle and the metadata 
    file object.

*/
{


    NTSTATUS status;

    UNREFERENCED_PARAMETER( CbdContext );
    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    //
    //  Sanity - we should only have a post operation for IRP_MN_CANCEL_DEVICE.
    //
    
    FLT_ASSERT( Cbd->Iopb->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE );

    //
    //  Sanity - IRP_MN_CANCEL_DEVICE cannot fail.
    //

    FLT_ASSERT( Cbd->IoStatus.Status == STATUS_SUCCESS );

    if( FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING ) ) {

        //
        //  We are draining. This means that we should not reacquire our 
        //  resources because the IRP may not have completed.
        //

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  The device removal was cancelled - reaquire our references to the
    //  metadata file handle and the metadata file object
    //

    status = FmmReacquireMetadataFileReferences( Cbd );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_METADATA_OPERATIONS,
                ("[Fmm]: Failed to re-open metadata with status 0x%x after cancel device removal.\n",
                 status) );

        //
        //  Sanity - we are now in a bad state. The removal has been
        //  cancelled but we have not been able to re-acquire references
        //  to our metadata file
        //
        //  It is always possible to fail with STATUS_INSUFFICIENT_RESOURCES
        //  so we should ignore that.
        //
        //  It is also possible to fail if the instance context was in a transition state
        //  so we should ignore STATUS_FILE_LOCK_CONFLICT too.
        //

        FLT_ASSERT( (status == STATUS_INSUFFICIENT_RESOURCES) ||
                    (status == STATUS_FILE_LOCK_CONFLICT) );

    }
    
    return FLT_POSTOP_FINISHED_PROCESSING;
}


