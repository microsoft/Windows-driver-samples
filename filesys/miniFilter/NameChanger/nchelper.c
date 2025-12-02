/*++

Copyright (c) 2008 - 2009  Microsoft Corporation

Module Name:

    nchelper.c

Abstract:

    Contains helper routines of general applicability across name changer.

Environment:

    Kernel mode

--*/

#include "nc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcAllocateEResource)
#pragma alloc_text(PAGE, NcCreateFileHelper)
#pragma alloc_text(PAGE, NcGetFileNameInformation)
#endif

_When_(Data == NULL, _Pre_satisfies_(FileObject != NULL && Instance != NULL))
_When_(FileObject == NULL || Instance == NULL, _Pre_satisfies_(Data != NULL))
NTSTATUS
NcGetFileNameInformation(
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_INSTANCE Instance,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION *FileNameInformation
    )
/*++

Routine Description:

    This function is a wrapper to call the correct variant of
    FltGetFileNameInformation depending on the information we happen to
    have available.

Arguments:

    Data - Pointer to the callback data structure associated with a request.
        This is optional, but if not specified, FileObject and Instance must
        be supplied.

    FileObject - Pointer to the file object to query a name on.  Optional,
        but if not supplied, Data must be supplied.

    Instance - Pointer to the instance of our filter to query the name on.
        Optional, but if not supplied, Data must be supplied.

    NameOptions - FLT_FILE_NAME_* flags for this request.

    FileNameInformation - On output, contains the file name information
        resulting from this query.  On failure, contents are undefined.
        On success, caller is responsible for releasing this with
        FltReleaseFileNameInformation.

Return Value:

    Returns the status of the operation.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    FLT_ASSERT( Data || FileObject );

    *FileNameInformation = NULL;

    if (ARGUMENT_PRESENT( Data )) {

        Status = FltGetFileNameInformation( Data,
                                            NameOptions,
                                            FileNameInformation );

    } else if (ARGUMENT_PRESENT( FileObject )) {

        Status = FltGetFileNameInformationUnsafe( FileObject,
                                                  Instance,
                                                  NameOptions,
                                                  FileNameInformation );
    //
    //  This should never happen, as either Data or FileObject must be non-NULL.
    //

    } else {

        FLT_ASSERT( FALSE );
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS
NcAllocateEResource(
    _Out_ PERESOURCE * OutputLock
    )
/*++

Routine Description:

    This function allocates a new ERESOURCE.

Arguments:

    OutputLock - On output, points to a newly allocated ERESOURCE.  This
        value is undefined on failure.  Caller is responsible for deleting
        this with a call to NcFreeEResource.

Return Value:

    Returns the status of the operation.

--*/
{
    NTSTATUS Status;

    PERESOURCE Lock = NULL;
    BOOLEAN DeleteLock = FALSE;

    PAGED_CODE();

    Lock = ExAllocatePoolZero( NonPagedPool,
                               sizeof(ERESOURCE),
                               NC_LOCK_TAG );

    if (Lock == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcAllocateEResourceCleanup;
    }

    Status = ExInitializeResourceLite( Lock );

    if (!NT_SUCCESS( Status )) {

        goto NcAllocateEResourceCleanup;
    }

    Status = STATUS_SUCCESS;
    DeleteLock = TRUE;
    *OutputLock = Lock;

NcAllocateEResourceCleanup:

    if (!NT_SUCCESS( Status )) {

        if (Lock) {

            if (DeleteLock) {

                ExDeleteResourceLite( Lock );
            }
            ExFreePoolWithTag( Lock, NC_LOCK_TAG );
        }
    }
    return Status;
}

VOID
NcFreeEResource(
    _In_ PERESOURCE Lock
    )
/*++

Routine Description:

    This function frees an ERESOURCE.

Arguments:

    Lock - Points to an ERESOURCE allocated previously via
        NcAllocateEResource that needs to be torn down.

Return Value:

    Returns the status of the operation.

--*/
{
    ExDeleteResourceLite( Lock );
    ExFreePoolWithTag( Lock, NC_LOCK_TAG );
}

NTSTATUS
NcCreateFileHelper (
    _In_ PFLT_FILTER Filter,
    _In_opt_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE FileHandle,
    _Outptr_opt_ PFILE_OBJECT *FileObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Flags,
    _In_opt_ PFILE_OBJECT ParentFileObject
    )
{

#if FLT_MGR_LONGHORN
    IO_DRIVER_CREATE_CONTEXT DriverContext;
#endif
    NTSTATUS Status;

    PAGED_CODE();

#if FLT_MGR_LONGHORN
    IoInitializeDriverCreateContext( &DriverContext );

    if (ARGUMENT_PRESENT( ParentFileObject )) {
        PTXN_PARAMETER_BLOCK TxnInfo;

        TxnInfo = IoGetTransactionParameterBlock( ParentFileObject );

        //
        //  If we have a FileObject, and if it has a transaction, pass the
        //  same transaction context along to our internal creates.
        //
        //  TxfInfo will be valid so long as the file object is valid, which
        //  it is assumed to be across the call to this routine.
        //

        DriverContext.TxnParameters = TxnInfo;
    }
#else
    UNREFERENCED_PARAMETER( ParentFileObject );
#endif

    Status = NcCreateFileEx2( Filter,
                              Instance,
                              FileHandle,
                              FileObject,
                              DesiredAccess,
                              ObjectAttributes,
                              IoStatusBlock,
                              AllocationSize,
                              FileAttributes,
                              ShareAccess,
                              CreateDisposition,
                              CreateOptions,
                              EaBuffer,
                              EaLength,
                              Flags,
#if FLT_MGR_LONGHORN
                              &DriverContext );
#else
                              NULL );
#endif

    return Status;
}


NTSTATUS
NcSetCancelCompletion(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PFLT_COMPLETE_CANCELED_CALLBACK CanceledCallback
    )
/*++

Routine Description:

    This sets the cancel routine attached to the request.  We
    synchronize against the request being cancelled via the system
    global cancel spinlock, and if the request is already cancelled,
    this function returns STATUS_CANCELLED to the caller to
    indicate that the caller must clean up the request.  Otherwise,
    the cancel routine is set and immediately upon return any
    cancellation will be via that mechanism.

    NOTE: This routine must be nonpaged, since we require
    synchronization with the system global cancel spinlock.

Arguments:

    Data - Pointer to the request we wish to assign cancellation to.

    CanceledCallback - Pointer to the functin to call upon
                       cancellation.

Return Value:

    STATUS_CANCELLED if the request should be completed by the
    caller, STATUS_SUCCESS to indicate that the cancellation
    routine is associated with the request.

--*/
{
    NTSTATUS Status;
    KIRQL OldIrql;

    IoAcquireCancelSpinLock( &OldIrql );

    if (FltIsIoCanceled( Data )) {
        IoReleaseCancelSpinLock( OldIrql );
        return STATUS_CANCELLED;

    }
    Status = FltSetCancelCompletion( Data, CanceledCallback );

    IoReleaseCancelSpinLock( OldIrql );
    return Status;
}


LONG
NcExceptionFilter (
    _In_ PEXCEPTION_POINTERS ExceptionPointer,
    _In_ BOOLEAN AccessingUserBuffer
    )
/*++

Routine Description:

    Exception filter to catch errors touching user buffers.

Arguments:

    ExceptionPointer - The exception record.

    AccessingUserBuffer - If TRUE, overrides FsRtlIsNtStatusExpected to allow
                          the caller to munge the error to a desired status.

Return Value:

    EXCEPTION_EXECUTE_HANDLER - If the exception handler should be run.

    EXCEPTION_CONTINUE_SEARCH - If a higher exception handler should take care of
                                this exception.

--*/
{
    NTSTATUS Status;

    Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    //  Certain exceptions shouldn't be dismissed within the namechanger filter
    //  unless we're touching user memory.
    //

    if (!FsRtlIsNtstatusExpected( Status ) &&
        !AccessingUserBuffer) {

        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

