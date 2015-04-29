/*++

Copyright (c) 2009  Microsoft Corporation

Module Name:

    ncdirnotify.c

Abstract:

    Contains routines to process user-initiated directory change notifications.
    Depending on path, we may need to suppress the real mapping and its
    children from generating notifications to the user, or inject changes from
    the user mapping to the user.  In some cases we can optimize this merely
    by transforming the real mapping to user mapping (if we are watching an
    ancestor of both paths.)

Environment:

    Kernel mode

--*/

#include "nc.h"

//
//  The following structures define a context which is allocated per sub-request.
//  We record the type of the subrequest, and any state that is subrequest
//  specific.
//
typedef enum _NC_NOTIFY_REQUEST_TYPE {
    NotifyUserRequest = 0,
    NotifyShadowRequest,
    NotifyMappingRequest
} NC_NOTIFY_REQUEST_TYPE;

typedef struct _NC_NOTIFY_REQUEST_CONTEXT {
    PNC_STREAM_HANDLE_CONTEXT UserHandleContext;
    PFILE_OBJECT FileObjectToDereference;
    NC_NOTIFY_REQUEST_TYPE RequestType;
    PFLT_CALLBACK_DATA Request;
} NC_NOTIFY_REQUEST_CONTEXT, *PNC_NOTIFY_REQUEST_CONTEXT;

NTSTATUS
NcAllocateNotifyRequestContext (
    _In_ PNC_STREAM_HANDLE_CONTEXT HandleContext,
    _In_ PFILE_OBJECT FileObject,
    _In_ NC_NOTIFY_REQUEST_TYPE RequestType,
    _In_ PFLT_CALLBACK_DATA Request,
    _Out_ PNC_NOTIFY_REQUEST_CONTEXT * NotifyRequestContext
    );

VOID
NcFreeNotifyRequestContext (
    _In_ PNC_NOTIFY_REQUEST_CONTEXT NotifyRequestContext
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostNotifyDirectorySafe (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

VOID
NcPostNotifyDirectoryReal (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PVOID CompletionContext
    );

NTSTATUS
NcDirNotifyTranslateBuffers (
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ BOOLEAN IgnoreCase,
    _In_ PUNICODE_STRING UserRequestName,
    _In_ PUNICODE_STRING OpenedName,
    _In_reads_bytes_(InputBufferLength) PFILE_NOTIFY_INFORMATION InputSystemBuffer,
    _Out_writes_bytes_to_(OutputBufferLength, *OutputBufferWritten) PFILE_NOTIFY_INFORMATION OutputUserBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG InputBufferConsumed,
    _Out_ PULONG OutputBufferWritten,
    _In_ BOOLEAN ReturnRealMappingPaths,
    _In_ BOOLEAN ReturnInMappingOnly
    );



NTSTATUS
NcBuildSubNotifyRequest (
    _In_ PFLT_CALLBACK_DATA PrimaryRequest,
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ PNC_STREAM_HANDLE_CONTEXT HandleContext,
    _In_ NC_NOTIFY_REQUEST_TYPE RequestType,
    _Out_ PFLT_CALLBACK_DATA * SubRequest,
    _Out_ PNC_NOTIFY_REQUEST_CONTEXT * NotifyRequestContext
    );

VOID
NcCleanupSubNotifyRequest (
    _In_ PFLT_CALLBACK_DATA SubRequest
    );

NTSTATUS
NcGetDestinationNotifyBuffer (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _Outptr_result_bytebuffer_maybenull_(*BufferSize) PVOID * DestBuffer,
    _Out_ PULONG BufferSize
    );

_Pre_satisfies_(*LockHeld)
_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(*HandleContext->Lock)
_When_(*LockHeld == FALSE, _Releases_lock_(_Global_critical_region_))
_When_(*LockHeld == FALSE, _Releases_lock_(*HandleContext->Lock))
VOID
NcNotifyAbort(
    _In_ PNC_STREAM_HANDLE_CONTEXT HandleContext,
    _In_ NTSTATUS Status,
    _Inout_ PBOOLEAN LockHeld
    );

VOID
NcNotifyCancelCallback(
    _In_ PFLT_CALLBACK_DATA Data
    );

VOID
NcCloseHandleWorkerRoutine (
    _In_ PFLT_GENERIC_WORKITEM WorkItem,
    _In_ PFLT_FILTER Filter,
    _In_ PVOID HandlePtr
    );

VOID
NcReissueNotifyRequestWorkerRoutine (
    _In_ PFLT_GENERIC_WORKITEM WorkItem,
    _In_ PFLT_FILTER Filter,
    _In_ PVOID RequestPtr
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcAllocateNotifyRequestContext)
#pragma alloc_text(PAGE, NcBuildSubNotifyRequest)
#pragma alloc_text(PAGE, NcCleanupSubNotifyRequest)
#pragma alloc_text(PAGE, NcCloseHandleWorkerRoutine)
#pragma alloc_text(PAGE, NcDirNotifyTranslateBuffers)
#pragma alloc_text(PAGE, NcFreeNotifyRequestContext)
#pragma alloc_text(PAGE, NcGetDestinationNotifyBuffer)
#pragma alloc_text(PAGE, NcNotifyAbort)
#pragma alloc_text(PAGE, NcNotifyCancelCallback)
#pragma alloc_text(PAGE, NcPreNotifyDirectory)
#pragma alloc_text(PAGE, NcPostNotifyDirectorySafe)
#pragma alloc_text(PAGE, NcPostNotifyDirectoryReal)
#pragma alloc_text(PAGE, NcReissueNotifyRequestWorkerRoutine)
#pragma alloc_text(PAGE, NcStreamHandleContextNotCleanup)
#pragma alloc_text(PAGE, NcStreamHandleContextNotCreate)
#pragma alloc_text(PAGE, NcStreamHandleContextNotClose)
#endif

#define NcRemoveTrailingSlashIfPresent( US )                             \
    FLT_ASSERT( (US)->Length > 0 );                                          \
    if ( (US)->Buffer[(US)->Length/sizeof(WCHAR) - 1] == NC_SEPARATOR) { \
        (US)->Length -= sizeof(WCHAR);                                   \
    } 


NTSTATUS
NcDirNotifyTranslateBuffers (
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ BOOLEAN IgnoreCase,
    _In_ PUNICODE_STRING UserRequestName,
    _In_ PUNICODE_STRING OpenedName,
    _In_reads_bytes_(InputBufferLength) PFILE_NOTIFY_INFORMATION InputSystemBuffer,
    _Out_writes_bytes_to_(OutputBufferLength, *OutputBufferWritten) PFILE_NOTIFY_INFORMATION OutputUserBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG InputBufferConsumed,
    _Out_ PULONG OutputBufferWritten,
    _In_ BOOLEAN ReturnRealMappingPaths,
    _In_ BOOLEAN ReturnInMappingOnly
    ) 
/*++

Routine Description:

    This routine is used to transform buffers from a filesystem view to the
    user view.  Depending on flags we either replace references to the real
    mapping with the user mapping, suppress all entries from the real mapping,
    or only return results from inside the real mapping.

Arguments:

    InstanceContext - Pointer to the context describing this instance of the
        filter.

    IgnoreCase - TRUE if comparisons should be case insensitive, FALSE if
        comparisons should be case sensitive.

    UserRequestName - The string for the path that the user opened and is
        requesting notifications on.

    OpenedName - The string for the path that we are processing notifications
        on.  "Typically" the same as UserRequestName, but may be different if
        we are merging notifications from the user's handle and combining
        from the real mapping.  In this case, OpenedName may refer to the
        path to the real mapping.

    InputSystemBuffer - The buffer we are processing from.  Note that this
        routine assumes the buffer is not volatile (cannot be externally
        modified.)  For this reason, the buffer is expected to be system
        buffered by the caller if it is not already.

        Note however that the contents of the buffer may have originated from
        a user buffer (via a memcpy), so although the contents are non-volatile,
        they are not to be trusted.

    OutputUserBuffer - The buffer we are returning munged results into.
        This buffer is expected to have been probed, and this function will
        catch and return any invalid buffer exceptions.

    InputBufferLength - Size, in bytes, of the input buffer.

    OutputBufferLength - Size, in bytes, of the output buffer.

    InputBufferConsumed - Pointer to a ULONG which will contain, on output,
        the number of bytes processed from the input buffer.  This may be
        zero, the length of the input buffer, or any value in between.
        This value is undefined on failure.

    OutputBufferWritten - Pointer to a ULONG which will contain, on output,
        the number of bytes written into the output buffer.  This may be
        zero, the length of the output buffer, or any value in between.
        This value is undefined on failure.

    ReturnRealMappingPaths - TRUE if this function should transform and
        return any paths that are within the real mapping.  If FALSE, we
        omit these entries.

    ReturnInMappingOnly - TRUE if this function should transform and return
        paths from within the real mapping only, and omit all other paths.
        FALSE if it should return all paths.  Only meaningful if
        ReturnRealMappingPaths is TRUE.

Return Value:

    The return value is the Status of the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NC_PATH_OVERLAP RealOverlap;

    //
    //  Pointers into the above buffers used as we read entries from
    //  one buffer and write them to the other.  We keep a pointer to
    //  the previous destination entry to allow us to terminate the
    //  list when done.
    //

    PFILE_NOTIFY_INFORMATION SourceEntry = NULL;
    PFILE_NOTIFY_INFORMATION DestEntry = NULL;
    PFILE_NOTIFY_INFORMATION PrevDestEntry = NULL;

    //
    //  A full path to the path returned from the filesystem that we are
    //  currently examining; the Remainder of that path if it turns out
    //  to be in a mapping; the transformed name if one is required.
    //

    UNICODE_STRING NameString = EMPTY_UNICODE_STRING;
    UNICODE_STRING Remainder;
    UNICODE_STRING MungedName = EMPTY_UNICODE_STRING;

    //
    //  A pointer to one of the above buffers that we intend on returning to
    //  the application.  Note that this may be NULL if an entry is being
    //  suppressed.
    //

    PUNICODE_STRING ReturnName;
    ULONG EntryLength;

    ULONG UlongResult;
    PVOID PointerResult;

    PAGED_CODE();

    SourceEntry = InputSystemBuffer;
    DestEntry = OutputUserBuffer;
    *InputBufferConsumed = 0;
    *OutputBufferWritten = 0;

    //
    //  This routine assumes it will only be called if there is work to do.
    //

    FLT_ASSERT( InputBufferLength && OutputBufferLength );

    try {

        while (SourceEntry) {

            //
            //  The path returned is relative to the handle used to request
            //  the service.  We now allocate and construct a full path name.
            //  Use safe math routines when consulting the buffer, since it is
            //  possible that the buffer contents were modified by malicious
            //  code before getting here.
            //

            EntryLength = OpenedName->Length +
                          sizeof(WCHAR);

            if (EntryLength >= MAXUSHORT) {

                Status = STATUS_OBJECT_PATH_INVALID;
                goto NcDirNotifyTranslateBuffersCleanup;
            }

            Status = RtlULongAdd( EntryLength,
                                  SourceEntry->FileNameLength,
                                  &EntryLength );

            if (!NT_SUCCESS( Status ) ||
                (EntryLength >= MAXUSHORT)) {

                Status = STATUS_OBJECT_PATH_INVALID;
                goto NcDirNotifyTranslateBuffersCleanup;
            }

            //
            //  Now check whether this entry walks off the end of the input buffer.
            //

            Status = RtlULongAdd( *InputBufferConsumed,
                                  FIELD_OFFSET(FILE_NOTIFY_INFORMATION, FileName),
                                  &UlongResult );

            if (NT_SUCCESS( Status )) {

                Status = RtlULongAdd( UlongResult,
                                      SourceEntry->FileNameLength,
                                      &UlongResult );
            }

            if (!NT_SUCCESS( Status ) ||
                (UlongResult > InputBufferLength)) {

                //
                //  We have an entry that walks off the end of the buffer.
                //  Since the buffer that we get from the filesystem is not
                //  system buffered, we cannot guarantee that a caller is
                //  not corrupting it in an in-flight request.  This
                //  condition should never occur without aforementioned
                //  corruption.
                //

                FLT_ASSERT( FALSE );
                Status = STATUS_INVALID_USER_BUFFER;
                goto NcDirNotifyTranslateBuffersCleanup;
            }

            if (EntryLength > NameString.MaximumLength) {

                if (NameString.Buffer != NULL) {
                    NcFreeUnicodeString( &NameString );
                }

                NameString.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                           EntryLength,
                                                           NC_TAG );

                if (NameString.Buffer == NULL) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto NcDirNotifyTranslateBuffersCleanup;
                }

                NameString.MaximumLength = (USHORT)EntryLength;
            }

            _Analysis_assume_(NameString.Buffer != NULL);
            RtlCopyMemory( NameString.Buffer,
                           OpenedName->Buffer,
                           OpenedName->Length );

            NameString.Buffer[OpenedName->Length / sizeof(WCHAR)] = NC_SEPARATOR;

            RtlCopyMemory( Add2Ptr( NameString.Buffer, OpenedName->Length + sizeof(WCHAR) ),
                           SourceEntry->FileName,
                           SourceEntry->FileNameLength );

            NameString.Length = (USHORT)EntryLength;

            //
            //  Now see if this name needs to be munged.
            //

            NcComparePath( &NameString,
                           &InstanceContext->Mapping.RealMapping,
                           &Remainder,
                           IgnoreCase,
                           TRUE,
                           &RealOverlap );

            if (RealOverlap.InMapping && ReturnRealMappingPaths) {

                //
                //  If the name is under the real mapping and we're returning
                //  paths under the real mapping, the query is a common
                //  ancestor of both real and user mappings.  In this case,
                //  we need to translate real mapping paths to user mapping
                //  paths.
                //

                Status = NcConstructPath( &InstanceContext->Mapping.UserMapping,
                                          &Remainder,
                                          TRUE,
                                          &MungedName );

                if (!NT_SUCCESS( Status )) {
                    goto NcDirNotifyTranslateBuffersCleanup;
                }

                ReturnName = &MungedName;

            } else if (ReturnInMappingOnly || RealOverlap.InMapping) {

                //
                //  If the name is in the real mapping but the caller is asking
                //  for a tree which is not an ancestor of the user mapping,
                //  then this name should not be visible under this tree.  If
                //  the name is an ancestor of the real mapping, but the caller
                //  only requires children, this name should not be visible
                //  under this tree either.
                //

                ReturnName = NULL;

            } else {

                ReturnName = &NameString;

            }

            if (ReturnName != NULL) {

                ULONG EntryLengthExact;

                //
                //  Since this API returns paths under the one the query was
                //  issued on, the path we have now had better be longer than
                //  that path length.  Even if we refer to the same object,
                //  our path includes a trailing slash.
                //

                FLT_ASSERT( ReturnName->Length > UserRequestName->Length );
                _Analysis_assume_( ReturnName->Length > UserRequestName->Length );

                //
                //  We take care not to copy the leading slash.  If we have
                //  a trailing slash as well as a leading slash, truncate the
                //  string.  This can occur when we're injecting an entry for
                //  the mapping.
                //

                if (ReturnName->Length - UserRequestName->Length > sizeof(WCHAR) &&
                    ReturnName->Buffer[ReturnName->Length/sizeof(WCHAR) - 1] == NC_SEPARATOR) {

                    ReturnName->Length -= sizeof(WCHAR);
                }

                EntryLengthExact = FIELD_OFFSET( FILE_NOTIFY_INFORMATION, FileName );
                EntryLengthExact += (ReturnName->Length - UserRequestName->Length - sizeof(WCHAR));
    
                EntryLength = AlignToSize( EntryLengthExact, 8);
    
                //
                //  We've done all we can.  Return now to let our caller deal
                //  with the remaining buffer.
                //

                Status = RtlULongAdd( EntryLength,
                                      *OutputBufferWritten,
                                      &UlongResult );

                if (!NT_SUCCESS( Status ) ||
                    (UlongResult > OutputBufferLength)) {

                    if (PrevDestEntry != NULL) {
                        PrevDestEntry->NextEntryOffset = 0;
                    }

                    SourceEntry = NULL;
                    DestEntry = NULL;
                    break;
                }
    
                //
                //  Copy the relative path name, taking care to exclude the
                //  initial slash.
                //

                DestEntry->FileNameLength = ReturnName->Length -
                                            UserRequestName->Length -
                                            sizeof(WCHAR);

                RtlCopyMemory( DestEntry->FileName,
                               Add2Ptr( ReturnName->Buffer, UserRequestName->Length + sizeof(WCHAR)),
                               ReturnName->Length - UserRequestName->Length - sizeof(WCHAR));
                DestEntry->Action = SourceEntry->Action;
                DestEntry->NextEntryOffset = EntryLength;
    
                //
                //  Advance the destination that we're writing new entries by
                //  however much we just consumed.
                //

                PrevDestEntry = DestEntry;
                *OutputBufferWritten += EntryLength;
                DestEntry = Add2Ptr( DestEntry, EntryLength );
    
                if (MungedName.Buffer != NULL) {
                    ExFreePoolWithTag( MungedName.Buffer, NC_GENERATE_NAME_TAG );
                    MungedName.Buffer = NULL;
                    MungedName.MaximumLength = MungedName.Length = 0;
                }
            }

            //
            //  Now calculate and advance the location we're reading and
            //  processing from.  If we're have no more buffer left, we're
            //  done.
            //

            //
            //  SourceEntry->NextEntryOffset is untrusted, since it may have been
            //  copied from a user-provided buffer.
            //
            
            EntryLength = SourceEntry->NextEntryOffset;

            PointerResult = Add2Ptr( SourceEntry, EntryLength );

            Status = RtlULongAdd( *InputBufferConsumed,
                                  EntryLength,
                                  InputBufferConsumed );

            //
            //  There's a problem if one of the following happened:
            //
            //  1) We overflowed when accounting for for consumed input buffer
            //  2) We wrapped when advancing SourceEntry
            //  3) PointerResult is not within InputSystemBuffer
            //
            
            if (!NT_SUCCESS( Status ) ||
                (PointerResult < (PVOID)SourceEntry) ||
                (PointerResult < Add2Ptr( InputSystemBuffer, sizeof(FILE_NOTIFY_INFORMATION) ))) {

                FLT_ASSERT( FALSE );

                Status = STATUS_INVALID_USER_BUFFER;
                goto NcDirNotifyTranslateBuffersCleanup;
            }
            
            SourceEntry = (PFILE_NOTIFY_INFORMATION)PointerResult;

            if ((EntryLength == 0)) {

                if (PrevDestEntry != NULL) {
                    PrevDestEntry->NextEntryOffset = 0;
                }

                *InputBufferConsumed = InputBufferLength;
                SourceEntry = NULL;
            }

            //
            //  If we've just advanced our next location beyond the end of the
            //  input buffer, or there isn't enough room in it for even a FILE_NOTIFY_INFORMATION
            //  structure, terminate the loop by setting SourceEntry to NULL so
            //  we can at least return the valid entries we have.
            //
            
            FLT_ASSERT( *InputBufferConsumed <= InputBufferLength );

            if ((SourceEntry != NULL) &&
                ((*InputBufferConsumed >= InputBufferLength) ||
                 (*InputBufferConsumed + FIELD_OFFSET( FILE_NOTIFY_INFORMATION, FileName ) > InputBufferLength))) {

                //
                //  Indicate to the caller that we consumed exactly the input buffer.
                //  Otherwise it may hold the possibly small remnant and come back
                //  in to this routine later with that remnant, causing us to overread
                //  the buffer.
                //

                *InputBufferConsumed = InputBufferLength;
                SourceEntry = NULL;
            }            
        }

    } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

        Status = STATUS_INVALID_USER_BUFFER;
    }

    //
    //  If we're succeeding, our list should be terminated.
    //

    FLT_ASSERT( PrevDestEntry == NULL ||
                !NT_SUCCESS( Status ) ||
                PrevDestEntry->NextEntryOffset == 0);

NcDirNotifyTranslateBuffersCleanup:

    if (MungedName.Buffer != NULL) {

        ExFreePoolWithTag( MungedName.Buffer, NC_GENERATE_NAME_TAG );
        MungedName.Buffer = NULL;
        MungedName.MaximumLength = MungedName.Length = 0;

    }

    if (NameString.Buffer != NULL) {

        NcFreeUnicodeString( &NameString );
    }

    return Status;
}

NTSTATUS
NcAllocateNotifyRequestContext (
    _In_ PNC_STREAM_HANDLE_CONTEXT HandleContext,
    _In_ PFILE_OBJECT FileObject,
    _In_ NC_NOTIFY_REQUEST_TYPE RequestType,
    _In_ PFLT_CALLBACK_DATA Request,
    _Out_ PNC_NOTIFY_REQUEST_CONTEXT * NotifyRequestContext
    )
/*++

Routine Description:

    This function allocates a new request context with the specified
    properties.  It references objects passed into it.

Arguments:

    HandleContext - Pointer to the handle context attached to the
                    user's handle.

    FileObject - The file that this request is being sent to.  This
                 will be different to the user's file object for
                 mapping requests.

    RequestType - Specifies which class of subrequest this context
                  is for.

    Request - Pointer to Fltmgr's notion of this request.

    NotifyRequestContext - On output, points to the newly allocated
                           context.  On failure, this value is
                           undefined.

Return Value:

    Status of the operation.

--*/
{
    PNC_NOTIFY_REQUEST_CONTEXT RequestContext;

    PAGED_CODE();

    RequestContext = ExAllocatePoolWithTag( PagedPool,
                                            sizeof(NC_NOTIFY_REQUEST_CONTEXT), 
                                            NC_TAG );

    if (RequestContext == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FltReferenceContext( HandleContext );
    RequestContext->UserHandleContext = HandleContext;

    if (ARGUMENT_PRESENT( FileObject )) {

        ObReferenceObject( FileObject );
        RequestContext->FileObjectToDereference = FileObject;
    }

    RequestContext->RequestType = RequestType;
    RequestContext->Request = Request;

    *NotifyRequestContext = RequestContext;

    return STATUS_SUCCESS;
}

VOID
NcFreeNotifyRequestContext (
    _In_ PNC_NOTIFY_REQUEST_CONTEXT NotifyRequestContext
    )
/*++

Routine Description:

    This function frees a request context previously allocated with
    NcAllocateNotifyRequestContext.  Since it dereferences objects
    referenced at allocate time, it must be called without locks
    held.

Arguments:

    NotifyRequestContext - The request context to tear down.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    FLT_ASSERT( ExIsResourceAcquiredSharedLite( NotifyRequestContext->UserHandleContext->Lock ) == 0 );

    if (NotifyRequestContext->FileObjectToDereference) {
        ObDereferenceObject( NotifyRequestContext->FileObjectToDereference );
    }

    FltReleaseContext( NotifyRequestContext->UserHandleContext );

    ExFreePoolWithTag( NotifyRequestContext, NC_TAG );
    
}

NTSTATUS
NcBuildSubNotifyRequest (
    _In_ PFLT_CALLBACK_DATA PrimaryRequest,
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ PNC_STREAM_HANDLE_CONTEXT HandleContext,
    _In_ NC_NOTIFY_REQUEST_TYPE RequestType,
    _Out_ PFLT_CALLBACK_DATA * SubRequest,
    _Out_ PNC_NOTIFY_REQUEST_CONTEXT * NotifyRequestContext
    )
/*++

Routine Description:

    This routine is used to create and initialize a new subrequest for
    directory change notifications.  We need to create a new subrequest
    when requests are being shadowed or when merging from multiple locations.
    For change notifications unrelated to the mapping or spanning both
    mapping locations, we can send the user's request to the filesystem and
    not create any subrequests.

Arguments:

    PrimaryRequest - Pointer to the user's request.  We use this to propagate
        settings into the sub request.

    Instance - Pointer to the instance of this filter.

    FileObject - Pointer to the file object for the subrequest.  This may be
        the same as the user's file object, or may be a file object to the
        parent of the real mapping.

    HandleContext - Pointer to the handle context attached to the
        user's handle.

    RequestType - Specifies which class of subrequest this context
        is for.

    SubRequest - Pointer to a location to contain the new request created by
        this routine.  This value is undefined on failure.

    NotifyRequestContext - On output, points to the newly allocated
        context.  On failure, this value is undefined.

Return Value:

    The return value is the Status of the operation.

--*/
{
    PVOID MyBuffer;
    ULONG BufferLength = PrimaryRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length;
    NTSTATUS Status;
    PFLT_CALLBACK_DATA NewRequest;

    PAGED_CODE();

    if (BufferLength > 0) {

        //
        //  Since this request isn't being sent to the filesystem and
        //  we'll complete it in arbitary context, we need to build a
        //  MDL now so we can copy to system address space rather than
        //  back to the originating usermode process.
        //
    
        Status = FltLockUserBuffer( PrimaryRequest );

        if (!NT_SUCCESS( Status )) {
    
            return Status;
        }

        //
        //  The #pragma is a notation to the static code analyzer to not worry
        //  that we're apparently leaking MyBuffer.  It goes in to the subrequest
        //  we are building and will be cleaned up by NcCleanupSubNotifyRequest.
        //  That routine gets called on error in this routine, and from
        //  NcPostNotifyDirectoryReal, which is the completion routine that will
        //  be called once the subrequest we are building is finished executing.
        //

#pragma warning(suppress: 6014)
        MyBuffer = ExAllocatePoolWithTag( PagedPool,
                                          BufferLength,
                                          NC_TAG );
    
        if (MyBuffer == NULL) {
    
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

    } else {

        MyBuffer = NULL;
    }

    Status = FltAllocateCallbackData( Instance,
                                      FileObject,
                                      &NewRequest );

    if (!NT_SUCCESS( Status )) {

        if (MyBuffer != NULL) {
            ExFreePoolWithTag( MyBuffer, NC_TAG );
        }

        return Status;
    }

    NewRequest->Iopb->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    NewRequest->Iopb->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
    NewRequest->Iopb->OperationFlags = PrimaryRequest->Iopb->OperationFlags;
    NewRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length = BufferLength;
    NewRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer = MyBuffer;
    NewRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.CompletionFilter =
            PrimaryRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.CompletionFilter;
    NewRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.Spare1 = 0;
    NewRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.Spare2 = 0;
    NewRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.MdlAddress = NULL;

    if (BufferLength > 0) {

        NewRequest->Flags |= FLTFL_CALLBACK_DATA_SYSTEM_BUFFER;
    }

    //
    //  Allocate the request context.  We do this last because we
    //  cannot safely drop references if we fail after this point.
    //

    Status = NcAllocateNotifyRequestContext( HandleContext,
                                             FileObject,
                                             RequestType,
                                             NewRequest,
                                             NotifyRequestContext );

    if (!NT_SUCCESS( Status )) {

        NcCleanupSubNotifyRequest( NewRequest );
        return Status;
    }

    *SubRequest = NewRequest;
    return STATUS_SUCCESS;
}

VOID
NcCleanupSubNotifyRequest (
    _In_ PFLT_CALLBACK_DATA SubRequest
    )
/*++

Routine Description:

    This routine is used to tear down a subrequest once we are done with it.

Arguments:

    SubRequest - Pointer to the request to tear down.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    if (SubRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer != NULL) {

        ExFreePoolWithTag( SubRequest->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer,
                           NC_TAG );
    }

    FltFreeCallbackData( SubRequest );
}

NTSTATUS
NcGetDestinationNotifyBuffer (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _Outptr_result_bytebuffer_maybenull_(*BufferSize) PVOID * DestBuffer,
    _Out_ PULONG BufferSize
    )
/*++

Routine Description:

    This routine is used to obtain the virtual address that we should use
    to access a particular request's buffer.  Depending on the request,
    this may be system buffered, system mapped but user exposed, or user
    VA.

Arguments:

    Data - The request whose VA we are trying to obtain/map.

    DestBuffer - Pointer to a pointer which will, on output, point to the
        buffer associated with this request.  On failure, this value is
        undefined.

    BufferSize - Pointer to a location which will, on output, contain the
        length of the user's buffer.  On failure, this value is undefined.

Return Value:

    The return value is the Status of the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    *BufferSize = Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length;

    *DestBuffer = NULL;

    //
    //  FltLockUserBuffer doesn't like being called with zero length buffers.
    //  If we have one of those, end now.
    //

    if (Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length == 0) {

        Status = STATUS_SUCCESS;
        goto NcGetDestinationNotifyBufferCleanup;
    }

    //
    //  If we are in our post routine, we expect the buffer to have already
    //  been locked if FltGetNewSystemBufferAddress does not exist.  If it
    //  does exist, we may have a MDL, or we may be obtaining the system
    //  buffer supplied by the filesystem.
    //
    //  If we are issuing our own requests, we can also consume the user
    //  buffer directly, since it will always be system mapped (it's a pool
    //  allocation.)
    //

    if (Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.MdlAddress) {

        Status = FltLockUserBuffer( Data );

        if (!NT_SUCCESS( Status )) {

            goto NcGetDestinationNotifyBufferCleanup;
        }

        *DestBuffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.MdlAddress,
                                                    NormalPagePriority | MdlMappingNoExecute );
        if (*DestBuffer == NULL) {

            //
            //  If this fails, we have the pages locked in RAM but don't
            //  have VA to map them to.  STATUS_NO_MEMORY is usually used to
            //  indicate VA exhaustion.
            //

            Status = STATUS_NO_MEMORY;
            goto NcGetDestinationNotifyBufferCleanup;
        }
    } else {

        if (NcGetNewSystemBufferAddress) {
            *DestBuffer = NcGetNewSystemBufferAddress( Data );
        } else {
            *DestBuffer = NULL;
        }

        //
        //  If we have no MDL, and the filesystem hasn't allocated a system
        //  buffer, there are two possibilities:
        //
        //  1. This is a request we created ourselves and was already system
        //     buffered.
        //
        //  2. We are returning within the original thread context (ie.,
        //     buffered results being returned to the caller instantly.)
        //     In this case, we can use the user's VA.
        //

        if (*DestBuffer == NULL) {

            *DestBuffer = Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer;

            if (NcGetNewSystemBufferAddress == NULL) {

                //
                //  On Win7 where we have FltGetNewSystemBufferAddress, we
                //  can grab the system buffer from the Irp.  Pre-Win7 we
                //  must lock the user's buffer, thereby ending up in the
                //  above code block.  If we roll our own, the buffers we
                //  use are in kernel space.  The rule here is, no user VA
                //  can be sent down without being locked first.
                //

                FLT_ASSERT( *DestBuffer > (PVOID)0x80000000 );

            } else {

                try {

                    if (Data->RequestorMode != KernelMode) {
                        ProbeForWrite( *DestBuffer, *BufferSize, sizeof( UCHAR ));
                    }

                } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

                    Status = STATUS_INVALID_USER_BUFFER;
                }

            }
        }

    }

NcGetDestinationNotifyBufferCleanup:

    return Status;
}


FLT_PREOP_CALLBACK_STATUS
NcPreNotifyDirectory(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Routine is invoked when the user wants to modify a directory for
    changes.  The user can specify to monitor any directory, and can
    opt to monitor an entire subtree.

    We can approach this problem in one of four ways:

    1. If the directory being monitored has no relation to either
       mapping, we can send it straight to the filesystem.

    2. If the directory being monitored is an ancestor of the real
       mapping but not the user mapping, we must suppress entries
       from the mapping.

    3. If the directory being monitored is an ancestor of both
       mappings, we can change the contents of the buffer from real
       to user to reflect the change.

    4. If the directory being monitored is an ancestor of the user
       mapping but not the real mapping, we must send down two
       requests, one for the mapping parent and one for the user's
       handle, and combine the results back to the user.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure
        containing opaque handles to this filter, instance, its
        associated volume and file object.

    CompletionContext - The context for the completion routine for
        this operation.  We set this to the handle context for
        directory change notifications.

Return Value:

    The return value is the Status of the operation.

--*/
{
    FLT_PREOP_CALLBACK_STATUS ReturnValue;
    NTSTATUS Status;

    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    PNC_STREAM_HANDLE_CONTEXT HandleContext = NULL;
    PNC_DIR_NOT_CONTEXT NotCtx = NULL;

    PFLT_FILE_NAME_INFORMATION FileNameInformation = NULL;

    NC_PATH_OVERLAP RealOverlap;
    NC_PATH_OVERLAP UserOverlap;

    BOOLEAN WatchTree = BooleanFlagOn( Data->Iopb->OperationFlags, SL_WATCH_TREE );
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );
    BOOLEAN UnlockContext = FALSE;
    ULONG SizeWeReturn = 0;
    PNC_NOTIFY_REQUEST_CONTEXT ShadowRequestContext = NULL;
    PNC_NOTIFY_REQUEST_CONTEXT MappingParentRequestContext = NULL;
    PFLT_CALLBACK_DATA NewShadowRequest = NULL;
    PFLT_CALLBACK_DATA NewMappingParentRequest = NULL;

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  If the FileObject has been through cleanup, fail now.  This check
    //  is to catch where a file was cleaned up, then a notification was
    //  called on it for the first time.  We don't want to start
    //  initializing our structures now if we don't have to.
    //

    if (FlagOn( FltObjects->FileObject->Flags, FO_CLEANUP_COMPLETE )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        Status = STATUS_NOTIFY_CLEANUP;
        SizeWeReturn = 0;

        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  Get our instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  Get the directory's name.
    //

    Status = NcGetFileNameInformation( Data,
                                       NULL,
                                       NULL,
                                       FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
                                       &FileNameInformation ); 

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreNotifyDirectoryCleanup;
    }

    Status = FltParseFileNameInformation( FileNameInformation );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  See if the directory is parent of either mapping.
    //

    NcComparePath( &FileNameInformation->Name,
                   &InstanceContext->Mapping.UserMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &UserOverlap );

    NcComparePath( &FileNameInformation->Name,
                   &InstanceContext->Mapping.RealMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &RealOverlap );

    //
    //  If we are watching within the mapping, we have no
    //  work to do.  Strings returned are relative to the
    //  handle used to query for this operation.
    //

    if (UserOverlap.InMapping || RealOverlap.InMapping) {

        Status = STATUS_SUCCESS;
        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  If this object has no relation to either mapping,
    //  we have nothing to do.
    //

    if (!UserOverlap.Ancestor && !RealOverlap.Ancestor) {

        Status = STATUS_SUCCESS;
        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  If we are not monitoring recursively, and this is a
    //  non-parent ancestor, we have no work to do.
    //

    if (!WatchTree &&
        (!UserOverlap.Parent && !RealOverlap.Parent)) {

        Status = STATUS_SUCCESS;
        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  Attach our handle context.  We use this to record
    //  state about which object we're monitoring, how we're
    //  monitoring it, and our outstanding subrequests.
    //

    Status = NcStreamHandleContextAllocAndAttach( FltObjects->Filter,
                                                  FltObjects->Instance,
                                                  FltObjects->FileObject,
                                                  &HandleContext );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreNotifyDirectoryCleanup;
    }

    FLT_ASSERT( HandleContext != NULL );

    NotCtx = &HandleContext->DirectoryNotificationContext;

    //
    //  Before looking at the context, we have to acquire the lock.
    //
    
    NcLockStreamHandleContext( HandleContext );
    UnlockContext = TRUE;

    //
    //  If the user's handle has gone through cleanup, don't allow this
    //  request.  We're not synchronized with the FO_CLEANUP_COMPLETE
    //  flag, but we are synchronized with the CleanupSeen flag, so we
    //  need to do this check now after acquiring the lock.
    //

    if (NotCtx->CleanupSeen ||
        FlagOn( FltObjects->FileObject->Flags, FO_CLEANUP_COMPLETE )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        Status = STATUS_NOTIFY_CLEANUP;
        SizeWeReturn = 0;

        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  Save away the path used on this handle, if we haven't already.
    //  This is done to avoid having to re-request it in post.
    //
    //  Note that NameChanger will not allow renames of ancestor
    //  components, which are also the only paths it intercepts
    //  directory change notifications on.  Accordingly, the path we
    //  capture here should not change for the lifetime of the
    //  notification.
    //

    if (NotCtx->UserRequestName.Buffer == NULL) {

        NotCtx->UserRequestName.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                                FileNameInformation->Name.Length,
                                                                NC_TAG );

        if (NotCtx->UserRequestName.Buffer == NULL) {

            ReturnValue = FLT_PREOP_COMPLETE;
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto NcPreNotifyDirectoryCleanup;

        }

        RtlCopyMemory( NotCtx->UserRequestName.Buffer,
                       FileNameInformation->Name.Buffer,
                       FileNameInformation->Name.Length );

        NotCtx->UserRequestName.MaximumLength =
            NotCtx->UserRequestName.Length =
            FileNameInformation->Name.Length;

        NcRemoveTrailingSlashIfPresent( &NotCtx->UserRequestName );

        NotCtx->IgnoreCase = IgnoreCase;

        FltReferenceContext( InstanceContext );
        NotCtx->InstanceContext = InstanceContext;
        NotCtx->CompletionFilter = Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.CompletionFilter;
        NotCtx->OperationFlags = Data->Iopb->OperationFlags;

    }

    //
    //  If the user had previously cancelled a request on this handle,
    //  clear that now.  We want to process all results from this
    //  point.
    //

    NotCtx->CancelSeen = FALSE;

    //
    //  TODO: The notify package can record a number of Irps for the
    //  same directory (with the same settings for all.)  To preserve
    //  this semantic, we need to save off whether we're watching the
    //  entire tree or not in our handle context, then keep a list
    //  of outstanding requests.  These will be completed by the
    //  filesystem on a first in, first out basis.  Or perhaps, we
    //  maintain the list and only send the filesystem a "representative"
    //  query, but maintain our own list and re-send to the filesystem
    //  if further user requests exist.
    //
    //  Bah.
    //

    //  Check if this ever occurs in practice
    FLT_ASSERT( NotCtx->UserRequest == NULL );
    if (NotCtx->UserRequest != NULL) {

        Status = STATUS_UNSUCCESSFUL;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  If we previously saw the filesystem indicate that the buffer was
    //  not large enough to return all results, but we did not have a user
    //  request to complete to that effect, let the user know now.
    //

    if (NotCtx->InsufficientBufferSeen) {

        NotCtx->InsufficientBufferSeen = FALSE;
        Status = STATUS_NOTIFY_ENUM_DIR;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreNotifyDirectoryCleanup;
    }

    //
    //  If we still have leftover changes from a previous call,
    //  return them now.  Note that we do not want to set this
    //  request as being the user's request in this case.
    //

    if (NotCtx->BufferToFree != NULL) {

        PVOID Buffer;
        ULONG BufferSize;

        FLT_ASSERT( NotCtx->BufferLength != 0 );

        Status = NcGetDestinationNotifyBuffer( Data, &Buffer, &BufferSize );

        //
        //  We were unable to lock/map the user's buffer, or the buffer
        //  was invalid.
        //

        if (!NT_SUCCESS( Status )) {

            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcPreNotifyDirectoryCleanup;
        }

        //
        //  If the user's buffer isn't big enough, tell them to rescan.
        //  Since we're telling them to rescan, tear down the buffer.
        //

        if (BufferSize < NotCtx->BufferLength) {

            ExFreePoolWithTag( NotCtx->BufferToFree, NC_TAG );
            NotCtx->BufferToFree = NULL;
            NotCtx->BufferLength = 0;

            ReturnValue = FLT_PREOP_COMPLETE;
            Status = STATUS_NOTIFY_ENUM_DIR;
            goto NcPreNotifyDirectoryCleanup;

        } else {

            try {

                RtlCopyMemory( Buffer,
                               NotCtx->BufferToFree,
                               NotCtx->BufferLength );

            } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

                Status = STATUS_INVALID_USER_BUFFER;
                goto NcPreNotifyDirectoryCleanup;
            }

            SizeWeReturn = NotCtx->BufferLength;

            ExFreePoolWithTag( NotCtx->BufferToFree, NC_TAG );
            NotCtx->BufferToFree = NULL;
            NotCtx->BufferLength = 0;

            ReturnValue = FLT_PREOP_COMPLETE;
            Status = STATUS_SUCCESS;

            goto NcPreNotifyDirectoryCleanup;
        }
    }

    NotCtx->UserRequest = Data;

    //
    //  If we are watching the parent of the real mapping,
    //  or an ancestor of it including subtree, but not
    //  an ancestor of the user mapping, we'll need to "filter"
    //  out notifications that a caller should not see.
    //

    if ((WatchTree && RealOverlap.Ancestor && !UserOverlap.Ancestor) ||
        (!WatchTree && RealOverlap.Parent && !UserOverlap.Parent)) {

        NotCtx->Mode = Filter;

        //
        //  TODO: Currently the lifetime of requests in filter mode is
        //  lock-step.  If we get here when the Irp is already sent to
        //  the filesystem, that implies that we have multiple async
        //  notify irps sent to us, and we either support having
        //  multiple notify irps sent to the filesystem, or we queue
        //  them here.
        //

        FLT_ASSERT( NotCtx->ShadowRequest == NULL );
        if (NotCtx->ShadowRequest == NULL) {

            Status = NcBuildSubNotifyRequest( Data,
                                              FltObjects->Instance,
                                              FltObjects->FileObject,
                                              HandleContext,
                                              NotifyShadowRequest,
                                              &NewShadowRequest,
                                              &ShadowRequestContext );

            if (!NT_SUCCESS( Status )) {

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }

        }

        //
        //  To synchronize this, we set the request in the context under
        //  cover of the context lock.  We then drop the lock to issue the
        //  IO.  The rule here is that IO should only be sent by its
        //  creator or its completor.
        //

        if (NewShadowRequest) {

            FLT_ASSERT( ShadowRequestContext );
            FLT_ASSERT( UnlockContext );

            //
            //  Set up the subrequest.  This is done before we set up a
            //  cancel callback to ensure that the cancel can do
            //  something meaningful if it's invoked.
            //

            NotCtx->ShadowRequest = NewShadowRequest;

            //
            //  Set a cancel routine on the user's request (which we intend
            //  on hanging on to.)  If the request is already cancelled,
            //  cancel our new request.
            //

            Status = NcSetCancelCompletion( Data, NcNotifyCancelCallback );

            if (!NT_SUCCESS( Status )) {

                NotCtx->ShadowRequest = NULL;

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }

            FLT_ASSERT( HandleContext != NULL );
            NcUnlockStreamHandleContext( HandleContext );
            UnlockContext = FALSE;

            //
            //  As soon as we send this, Data/NotCtx->UserRequest is
            //  no longer guaranteed to be valid.  Fltmgr will call
            //  our completion routine even on failure.  Regardless
            //  of what happens here, we do not need to worry about
            //  cleaning things up.
            //

            Status = FltPerformAsynchronousIo( NotCtx->ShadowRequest,
                                               NcPostNotifyDirectoryReal,
                                               ShadowRequestContext );

        }

        //
        //  Don't free the request or its context.  These belong to the
        //  request now.
        //

        ShadowRequestContext = NULL;
        NewShadowRequest = NULL;

        //
        //  We return pending here in all cases.  Either the filesystem
        //  pended the request (and we should too), or the filesystem
        //  completed it inline, in which case our completion routine has
        //  already completed our 'pending' request, so we'd better pend it.
        //
        //  If our request failed, Fltmgr will call our completion routine
        //  which will also propagate the failure to the user request and
        //  complete it.
        //

        ReturnValue = FLT_PREOP_PENDING;

    } else

    //
    //  If we are watching the tree of a mutual ancestor,
    //  or just the directory of a mutal parent, or within
    //  the mapping itself, we'll need to transform names.
    //

    if ((WatchTree && RealOverlap.Ancestor && UserOverlap.Ancestor) ||
        (RealOverlap.Parent && UserOverlap.Parent)) {

        //
        //  Filesystems may switch a directory change notify request
        //  to system buffered.  On Win7, we can obtain the resulting
        //  system buffer from FltGetNewSystemBufferAddress.  Prior
        //  to this we can't obtain it, so we must lock the user
        //  buffer now so the filesystem can obtain kernel VA and will
        //  not attempt to system buffer the request.
        //

        if (NcGetNewSystemBufferAddress == NULL) {

            Status = FltLockUserBuffer( Data );
            if (!NT_SUCCESS( Status )) {

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }

            if (MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.MdlAddress,
                                              NormalPagePriority | MdlMappingNoExecute ) == NULL) {

                Status = STATUS_NO_MEMORY;
                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }
        }

        //
        //  Allocate a context that describes this subrequest.
        //

        Status = NcAllocateNotifyRequestContext( HandleContext,
                                                 FltObjects->FileObject,
                                                 NotifyUserRequest,
                                                 Data,
                                                 &ShadowRequestContext );

        if (!NT_SUCCESS( Status )) {

            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcPreNotifyDirectoryCleanup;
        }

        *CompletionContext = ShadowRequestContext;
        ShadowRequestContext = NULL;

        //
        //  We have to do something with this call, so tell
        //  fltmgr to call us back.
        //

        ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

        NotCtx->Mode = Munge;

    } else

    //
    //  If we are watching the tree of an ancestor of the
    //  user mapping or just the directory of the parent
    //  of the user mapping, which is not an ancestor of
    //  the real mapping, we need to merge two different
    //  notifications into one.
    //

    if ((WatchTree && UserOverlap.Ancestor && !RealOverlap.Ancestor) ||
        (!WatchTree && UserOverlap.Parent && !RealOverlap.Parent)) {

        //
        //  If we already have a handle to the mapping parent, use it.
        //  If we don't have one, drop our lock and proceed to open it.
        //

        if (NotCtx->RealParentHandle == NULL) {

            OBJECT_ATTRIBUTES MappingParentAttributes;
            HANDLE MappingParentHandle = NULL;
            PFILE_OBJECT MappingParentFileObject = NULL;
            PWSTR MappingParentName;
            IO_STATUS_BLOCK MappingParentStatusBlock;
            PFLT_FILE_NAME_INFORMATION FileInfoInternalHandle = NULL;
            PFLT_GENERIC_WORKITEM MappingParentWorkItem = NULL;

            //
            //  Since we haven't opened a handle yet, we assume no requests
            //  can be outstanding which could race with us.  Even if the
            //  directory being used has been renamed so we're going into
            //  this path after being through one of the other paths, we
            //  should be in lock-step until we enter Merge mode, so no
            //  requests should exist.
            //

            FLT_ASSERT( NotCtx->ShadowRequest == NULL &&
                        NotCtx->MappingRequest == NULL );

            FLT_ASSERT( UnlockContext );
            NcUnlockStreamHandleContext( HandleContext );
            UnlockContext = FALSE;

            //
            //  Open the parent of the mapping.  We'll need to return results
            //  from both the user's handle and our own.  Note that we open
            //  the parent specifically so that a caller can monitor changes
            //  to the mapping itself.
            //
        
            InitializeObjectAttributes( &MappingParentAttributes,
                                        &InstanceContext->Mapping.RealMapping.LongNamePath.ParentPath,
                                        OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                        NULL,
                                        NULL);
        
            Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                         FltObjects->Instance,                 // InstanceOffsets
                                         &MappingParentHandle,                 // Returned Handle
                                         &MappingParentFileObject,             // Returned FileObject
                                         FILE_READ_ATTRIBUTES|FILE_TRAVERSE,   // Desired Access
                                         &MappingParentAttributes,             // object attributes
                                         &MappingParentStatusBlock,            // Returned IOStatusBlock
                                         0,                                    // Allocation Size
                                         FILE_ATTRIBUTE_NORMAL,                // File Attributes
                                         0,                                    // Share Access
                                         FILE_OPEN,                            // Create Disposition
                                         FILE_DIRECTORY_FILE,                  // Create Options
                                         NULL,                                 // Ea Buffer
                                         0,                                    // EA Length
                                         IO_IGNORE_SHARE_ACCESS_CHECK,         // Flags
                                         FltObjects->FileObject );             // Transaction info
           
            if (!NT_SUCCESS( Status )) {

                //
                //  This filter enforces the existence of mapping parents
                //  at attach time, and will prevent them being deleted or
                //  renamed.
                //

                FLT_ASSERT( Status != STATUS_OBJECT_PATH_NOT_FOUND &&
                            Status != STATUS_OBJECT_NAME_NOT_FOUND );

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;

            }

            Status = NcGetFileNameInformation( NULL,
                                               MappingParentFileObject,
                                               FltObjects->Instance,
                                               FLT_FILE_NAME_OPENED |
                                                   FLT_FILE_NAME_QUERY_DEFAULT |
                                                   FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                               &FileInfoInternalHandle );
            if (!NT_SUCCESS( Status )) {

                FltClose( MappingParentHandle );
                ObDereferenceObject( MappingParentFileObject );

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }

            Status = FltParseFileNameInformation( FileInfoInternalHandle );
            if (!NT_SUCCESS( Status )) {

                FltClose( MappingParentHandle );
                ObDereferenceObject( MappingParentFileObject );
                FltReleaseFileNameInformation( FileInfoInternalHandle );

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }

            MappingParentName = ExAllocatePoolWithTag( PagedPool,
                                                       FileInfoInternalHandle->Name.Length,
                                                       NC_TAG );

            if (MappingParentName == NULL) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

                FltClose( MappingParentHandle );
                ObDereferenceObject( MappingParentFileObject );
                FltReleaseFileNameInformation( FileInfoInternalHandle );

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }

            MappingParentWorkItem = FltAllocateGenericWorkItem();
            if (MappingParentWorkItem == NULL) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

                FltClose( MappingParentHandle );
                ObDereferenceObject( MappingParentFileObject );
                FltReleaseFileNameInformation( FileInfoInternalHandle );
                ExFreePoolWithTag( MappingParentName, NC_TAG );

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }


            NcLockStreamHandleContext( HandleContext );
            UnlockContext = TRUE;

            //
            //  We could be racing with another thread attempting to open the
            //  mapping.  If, after having reacquired the lock we still see
            //  no mapping handle, use ours.  If another thread beat us to it,
            //  tear down ours and use theirs.
            //

            if (NotCtx->RealParentHandle == NULL &&
                !NotCtx->CleanupSeen) {

                //
                //  Store our object into the context.
                //

                FLT_ASSERT( NotCtx->RealParentFileObject == NULL &&
                            NotCtx->RealParentCloseWorkItem == NULL &&
                            NotCtx->MappingParentName.Buffer == NULL );

                NotCtx->RealParentHandle = MappingParentHandle;
                NotCtx->RealParentFileObject = MappingParentFileObject;
                NotCtx->RealParentCloseWorkItem = MappingParentWorkItem;

                NotCtx->MappingParentName.Buffer = MappingParentName;
                NotCtx->MappingParentName.MaximumLength = FileInfoInternalHandle->Name.Length;

                RtlCopyMemory( NotCtx->MappingParentName.Buffer,
                               FileInfoInternalHandle->Name.Buffer,
                               FileInfoInternalHandle->Name.Length );

                NotCtx->MappingParentName.Length = FileInfoInternalHandle->Name.Length;

                NcRemoveTrailingSlashIfPresent( &NotCtx->MappingParentName );

                FltReleaseFileNameInformation( FileInfoInternalHandle );

            } else {

                //
                //  Another thread beat us to the punch, or our services are
                //  no longer required.  Tear down our state and reload from
                //  theirs.
                //
                //  Really, we can't race here (currently) because:
                //
                //  1. We do not support multiple outstanding requests on one
                //     handle (if attempted, we already failed.)
                //
                //  2. This must be the first request on this handle, or else
                //     our mapping parent handle already existed.
                //
                //  3. We have not yet set up cancellation on this request.
                //
                //  The only way to be here is if a cleanup request has
                //  occurred and we're aborting.
                //  

                FLT_ASSERT( NotCtx->CleanupSeen );

                NcUnlockStreamHandleContext( HandleContext );
                UnlockContext = FALSE;

                FltClose( MappingParentHandle );
                ObDereferenceObject( MappingParentFileObject );
                ExFreePoolWithTag( MappingParentName, NC_TAG );
                FltReleaseFileNameInformation( FileInfoInternalHandle );
                FltFreeGenericWorkItem( MappingParentWorkItem );

                NcLockStreamHandleContext( HandleContext );
                UnlockContext = TRUE;

                //
                //  Another thread beat us to creating the file object, but
                //  it was subsequently torn down while we dropped our lock.
                //  This can happen if the user handle is closed, in which
                //  case we expect CleanupSeen to be set and we can safely
                //  leave.  Return pending because the cleanup completed our
                //  request.
                //

                if (NotCtx->RealParentHandle == NULL) {

                    FLT_ASSERT( NotCtx->CleanupSeen );

                    ReturnValue = FLT_PREOP_PENDING;
                    goto NcPreNotifyDirectoryCleanup;
                }
            }

            MappingParentHandle = NULL;
            MappingParentFileObject = NULL;
            MappingParentName = NULL;

            //
            //  This relationship may have changed while we dropped the
            //  lock, but this can't happen in practice today:
            //
            //  1. We do not support multiple outstanding requests on one
            //     handle (if attempted, we already failed.)
            //
            //  2. This must be the first request on this handle, or else
            //     our mapping parent handle already existed.
            //
            //  3. We have not yet set up cancellation on this request.
            //
            //  4. If the user handle were closed, we failed out above.
            //
            //  If these change and we need to revalidate state, we may
            //  need to clear cancel state, return insufficient buffer,
            //  or return buffered data here.
            //

            FLT_ASSERT( NotCtx->UserRequest == Data );

            FLT_ASSERT( NotCtx->ShadowRequest == NULL &&
                        NotCtx->MappingRequest == NULL &&
                        !NotCtx->CleanupSeen &&
                        !NotCtx->CancelSeen &&
                        !NotCtx->InsufficientBufferSeen &&
                        NotCtx->BufferToFree == NULL &&
                        NotCtx->BufferLength == 0 );

        }

        FLT_ASSERT( UnlockContext );
        FLT_ASSERT( NotCtx->RealParentHandle != NULL &&
                    NotCtx->RealParentFileObject != NULL &&
                    NotCtx->RealParentCloseWorkItem != NULL );

        NotCtx->Mode = Merge;

        //
        //  Now create our child requests.  We need to watch for changes on
        //  both the user's handle and on the mapping parent handle.  While
        //  this process is ongoing, we're trying to drop/reacquire the lock
        //  as few times as possible, so we set up both requests at once,
        //  then issue them together.
        //
        //  If this is the second (or subsequent) time notifications are
        //  being requested, we may already have subrequests issued to the
        //  filesystem.  In that case, we leave the existing requests in
        //  place.  Any request not already in place we create ourselves
        //  now.
        //

        if (NotCtx->ShadowRequest == NULL) {

            Status = NcBuildSubNotifyRequest( Data,
                                              FltObjects->Instance,
                                              FltObjects->FileObject,
                                              HandleContext,
                                              NotifyShadowRequest,
                                              &NewShadowRequest,
                                              &ShadowRequestContext );

            if (!NT_SUCCESS( Status )) {

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }

        }

        if (NotCtx->MappingRequest == NULL) {

            Status = NcBuildSubNotifyRequest( Data,
                                              FltObjects->Instance,
                                              NotCtx->RealParentFileObject,
                                              HandleContext,
                                              NotifyMappingRequest,
                                              &NewMappingParentRequest,
                                              &MappingParentRequestContext );

            if (!NT_SUCCESS( Status )) {

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }
        }

        //
        //  To synchronize this, we set the request in the context under cover
        //  of the context lock.  We then drop the lock to issue the IO.
        //  The rule here is that IO should only be sent by its creator or its
        //  completor.
        //

        if (NewShadowRequest || NewMappingParentRequest) {

            FLT_ASSERT( UnlockContext );

            if (NewShadowRequest) {
                NotCtx->ShadowRequest = NewShadowRequest;
            }

            if (NewMappingParentRequest) {
                NotCtx->MappingRequest = NewMappingParentRequest;
            }

            //
            //  Set a cancel routine on the user's request (which we intend on
            //  hanging on to.)
            //

            Status = NcSetCancelCompletion( Data, NcNotifyCancelCallback );

            if (!NT_SUCCESS( Status )) {

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreNotifyDirectoryCleanup;
            }

            FLT_ASSERT( HandleContext != NULL );
            NcUnlockStreamHandleContext( HandleContext );
            UnlockContext = FALSE;

            //
            //  Note that as soon as we drop the lock, the user's request is
            //  no longer guaranteed to be valid.  Any previously outstanding
            //  IO may complete and take it away from us.
            //

            if (NewShadowRequest) {

                Status = FltPerformAsynchronousIo( NotCtx->ShadowRequest,
                                                   NcPostNotifyDirectoryReal,
                                                   ShadowRequestContext );

                ShadowRequestContext = NULL;
                NewShadowRequest = NULL;


                //
                //  If we failed and have a second request, grab the lock
                //  again to enable our cleanup code to tear it down.
                //

                if (!NT_SUCCESS( Status ) && NewMappingParentRequest) {

                    NcLockStreamHandleContext( HandleContext );
                    UnlockContext = TRUE;

                    ReturnValue = FLT_PREOP_PENDING;
                    goto NcPreNotifyDirectoryCleanup;

                }
            }

            if (NewMappingParentRequest) {

                Status = FltPerformAsynchronousIo( NotCtx->MappingRequest,
                                                   NcPostNotifyDirectoryReal,
                                                   MappingParentRequestContext );

                MappingParentRequestContext = NULL;
                NewMappingParentRequest = NULL;
            }
        }

        ReturnValue = FLT_PREOP_PENDING;

    }

    FLT_ASSERT( NotCtx->Mode != Uninitialized );

    FLT_ASSERT( NotCtx->InstanceContext != NULL );


NcPreNotifyDirectoryCleanup:

    if (ReturnValue == FLT_PREOP_COMPLETE) {
 
        //
        //  We need to write back results of query.
        //
 
        Data->IoStatus.Status = Status;
 
        if (NT_SUCCESS( Status )) {
 
            //success
            Data->IoStatus.Information = SizeWeReturn;

        } else {
 
            //failure
            Data->IoStatus.Information = 0;

        }

        //
        //  Since we're completing the request, remove it from the notify
        //  context structure.  Note that since we're completing it, it
        //  must still be valid and owned by us, so we're absolutely
        //  entitled (and required!) to remove it.
        //

        if (HandleContext != NULL) {

            if (!UnlockContext) {
                NcLockStreamHandleContext( HandleContext );
                UnlockContext = TRUE;
            }

            if (NotCtx->UserRequest == Data) {
                NotCtx->UserRequest = NULL;
            }
        }
    }

    if (NewShadowRequest) {

        //
        //  We should only be cleaning this up if we hold the lock.
        //

        FLT_ASSERT( UnlockContext );

        //
        //  Either this request is still set in the context (and we'll
        //  take care of that), or it's been cancelled and hence
        //  removed already.  In either case, we still need to tear it
        //  down.
        //

        FLT_ASSERT( NotCtx->ShadowRequest == NULL ||
                    NotCtx->ShadowRequest == NewShadowRequest );

        NcCleanupSubNotifyRequest( NewShadowRequest );
        NotCtx->ShadowRequest = NewShadowRequest = NULL;
    }

    if (NewMappingParentRequest) {

        //
        //  We should only be cleaning this up if we hold the lock.
        //

        FLT_ASSERT( UnlockContext );

        //
        //  Either this request is still set in the context (and we'll
        //  take care of that), or it's been cancelled and hence
        //  removed already.  In either case, we still need to tear it
        //  down.
        //

        FLT_ASSERT( NotCtx->MappingRequest == NULL ||
                    NotCtx->MappingRequest == NewMappingParentRequest );

        NcCleanupSubNotifyRequest( NewMappingParentRequest );
        NotCtx->MappingRequest = NewMappingParentRequest = NULL;
    }
 
    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }
 
    if (UnlockContext) {
 
        FLT_ASSERT( HandleContext != NULL );
        NcUnlockStreamHandleContext( HandleContext );
    }

    //
    //  Now that we've dropped the lock and can issue IO, tear down any
    //  request contexts from failed requests.
    //

    if (ShadowRequestContext) {
        NcFreeNotifyRequestContext( ShadowRequestContext );
    }

    if (MappingParentRequestContext) {
        NcFreeNotifyRequestContext( MappingParentRequestContext );
    }
 
    if (HandleContext != NULL) {

        FltReleaseContext( HandleContext );
    }
 
    if (FileNameInformation != NULL) {

        FltReleaseFileNameInformation( FileNameInformation );
    }

    return ReturnValue;
}


VOID
NcPostNotifyDirectoryReal (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PVOID CompletionContext
    )
/*++

Routine Description:

    Routine is invoked after the filesystem has returned a change
    notification, or if we are processing a cancellation for an
    outstanding change notification.

    This routine is guaranteed to be called <= APC level.  It is
    still not really safe to issue IO from here, since that may
    require us to be at passive level.  We also have no guarantees
    that TopLevelIrp == NULL at this point (and in many cases, we
    wouldn't expect it to be.)  Accordingly, this function should
    not call back into the filesystem directly itself.

Arguments:

    Data - Pointer to the request.  This could be the user's request,
        or it could be a subrequest we created for ourselves.

    CompletionContext - The context for the completion routine for
        this operation.  We set this to the handle context for
        directory change notifications.

Return Value:

    None.  If we are completing a request, any errors will be
    associated with that request.

--*/
{
    NTSTATUS Status;

    PFILE_NOTIFY_INFORMATION SourceBuffer = NULL;
    PFILE_NOTIFY_INFORMATION DestBuffer;

    PNC_INSTANCE_CONTEXT InstanceContext;
    PNC_NOTIFY_REQUEST_CONTEXT RequestContext = (PNC_NOTIFY_REQUEST_CONTEXT)CompletionContext;
    PNC_STREAM_HANDLE_CONTEXT HandleContext = RequestContext->UserHandleContext;
    PNC_DIR_NOT_CONTEXT NotCtx = NULL;

    ULONG SizeActuallyReturned = (ULONG)Data->IoStatus.Information;
    ULONG BufferSize;
    ULONG InputConsumed;
    ULONG SizeWeReturn;

    BOOLEAN UnlockContext = FALSE;
    BOOLEAN CompleteUserRequest = TRUE;
    BOOLEAN ReissuedRequest = FALSE;

    PAGED_CODE();

    NotCtx = &HandleContext->DirectoryNotificationContext;

    //
    //  Before looking at the context, we have to acquire the lock.
    //
    
    NcLockStreamHandleContext( HandleContext );
    UnlockContext = TRUE;

    //
    //  Perform debug-only sanity checking and set up our state so we
    //  can quickly tell which kind of request we're dealing with.
    //

    FLT_ASSERT( NotCtx->Mode == Filter || NotCtx->Mode == Munge || NotCtx->Mode == Merge );
    FLT_ASSERT( RequestContext->Request == Data );

    //
    //  If the user's handle has gone through cleanup, and we're completing
    //  it, make sure it fails correctly.  Make sure we don't propagate any
    //  success or meaningful information beyond this point.
    //

    if (NotCtx->CleanupSeen) {
        if (NotCtx->UserRequest == NULL) {
            CompleteUserRequest = FALSE;
        } else {
            Status = STATUS_NOTIFY_CLEANUP;
            SizeWeReturn = 0;
        }
        goto NcPostNotifyDirectoryRealCleanup;
    }

    //
    //  If the user has cancelled their request, make sure any completing
    //  subrequests are failed with STATUS_CANCELLED.  This is primarily to
    //  ensure that we don't reissue requests post-cancel (which will never
    //  be cancelled.)
    //

    if (NotCtx->CancelSeen) {

        Data->IoStatus.Status = STATUS_CANCELLED;
    }

    //
    //  Flow any failures back to the user's request, if we have one.
    //

    if (!NT_SUCCESS( Data->IoStatus.Status ) ||
        Data->IoStatus.Status == STATUS_NOTIFY_CLEANUP ||
        Data->IoStatus.Status == STATUS_NOTIFY_ENUM_DIR ) {

        Status = Data->IoStatus.Status;
        SizeWeReturn = 0;

        if (NotCtx->UserRequest == NULL) {

            CompleteUserRequest = FALSE;

            if (Data->IoStatus.Status == STATUS_NOTIFY_ENUM_DIR) {

                //
                //  If the filesystem is trying to tell the app to rescan,
                //  but we don't have the app's request right now, make a
                //  note to tell the app that it must rescan as soon as it
                //  asks us again.
                //
    
                NotCtx->InsufficientBufferSeen = TRUE;
            }
        }
        goto NcPostNotifyDirectoryRealCleanup;
    }

    FLT_ASSERT( NotCtx->Mode != Merge ||
                (NotCtx->RealParentFileObject != NULL &&
                 NotCtx->UserRequestName.Buffer != NULL ));

    FLT_ASSERT( Data == NotCtx->UserRequest ||
                Data == NotCtx->ShadowRequest ||
                Data == NotCtx->MappingRequest );

    InstanceContext = NotCtx->InstanceContext;

    Status = NcGetDestinationNotifyBuffer( Data, &DestBuffer, &BufferSize );
    if (!NT_SUCCESS( Status )) {

        //
        //  This can fail due no pages/VA to lock the user's buffer, or a
        //  'user buffer' that points to kernel space.  By the time we get
        //  here, we expect that our pages are either already locked or we
        //  do not need to lock them; and we expect that requests we
        //  generate internally are based on pool and should not require VA
        //  to map, nor be usermode pointers into kernel space.  Really, the
        //  only time this should fail is if we have no VA to map the user's
        //  request.
        //
        //  If this assumption is wrong, we need to be smarter about whether
        //  we want to complete the user's request in this path or not.
        //  

        FLT_ASSERT( Data == NotCtx->UserRequest );
        goto NcPostNotifyDirectoryRealCleanup;
    }

    FLT_ASSERT( SizeActuallyReturned <= BufferSize );

    //
    //  If we have a zero-length buffer, the filesystem should not have
    //  returned STATUS_SUCCESS.  We still try to tolerate this
    //  condition regardless.
    //

    FLT_ASSERT( BufferSize > 0 );

    //
    //  If the filesystem returned something, go ahead and try to munge it.
    //

    if (SizeActuallyReturned > (ULONG)FIELD_OFFSET( FILE_NOTIFY_INFORMATION, FileName )) {

        //
        //  Allocate a new buffer and copy the contents.  Note that this is
        //  particularly important with this call, since it's not always 
        //  system buffered; the contents are free to change underneath us.
        //  This allocation protects us against that, but we still must be
        //  paranoid touching the buffer, since we cannot trust that it has
        //  any integrity at this point.
        //

        SourceBuffer = ExAllocatePoolWithTag( PagedPool,
                                              SizeActuallyReturned,
                                              NC_TAG );

        if (SourceBuffer == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto NcPostNotifyDirectoryRealCleanup;
        }

        try {

            //
            //  The #pragma is a notation to the static code analyzer that the
            //  buffer sizes involved have been computed correctly so no buffer
            //  overrun is possible here.
            //
#pragma warning(suppress:6385)
            RtlCopyMemory( SourceBuffer, DestBuffer, SizeActuallyReturned );

        } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

            Status = STATUS_INVALID_USER_BUFFER;
            goto NcPostNotifyDirectoryRealCleanup;
        }

        //
        //  Now that we have a copy of the filesystem data, we need to
        //  transform it and return it to the user.  So now we point our
        //  destination buffers at the original request if we own it, if it
        //  wasn't already.
        //
        //  When merging from the mapping and non-mapping paths, we may have
        //  completed the user's request from one path and then receive
        //  notification on the second.  In this case, we buffer the
        //  notification so the caller can receieve it again next call.  We
        //  do not expect to need more than one buffer.  If this overflows,
        //  just tell the caller their buffer was too small.
        //

        if (RequestContext->RequestType == NotifyShadowRequest ||
            RequestContext->RequestType == NotifyMappingRequest) {

            if (NotCtx->UserRequest != NULL) {

                Status = NcGetDestinationNotifyBuffer( NotCtx->UserRequest,
                                                       &DestBuffer,
                                                       &BufferSize );

                if (!NT_SUCCESS( Status )) {
                    goto NcPostNotifyDirectoryRealCleanup;
                }

            } else {

                FLT_ASSERT( NotCtx->Mode == Merge );

                //
                //  We have at most two outstanding child requests, and if
                //  one completes we'll complete the master request,
                //  buffering with the second.  We therefore only need one
                //  buffer - most of the time.
                //
                //  There is a case where one buffer is insufficient: if the
                //  user issues a cancel while both child requests are
                //  completing, we may have a cancelled user request and two
                //  incoming pieces of data.  In this case, it's safe to
                //  throw one away since the act of cancelling implies the
                //  user doesn't expect to see everything.  However in that
                //  case we should have failed out above, and should never
                //  get here.
                //

                FLT_ASSERT( NotCtx->BufferToFree == NULL );
                if (NotCtx->BufferToFree != NULL) {

                    ExFreePoolWithTag( NotCtx->BufferToFree, NC_TAG );
                    NotCtx->BufferToFree = NULL;
                    NotCtx->BufferLength = 0;
                }

                //
                //  There is no user request, don't complete it.
                //

                CompleteUserRequest = FALSE;

                if (BufferSize > 0) {

                    //
                    //  Save away the output to our buffer.  Note that we
                    //  process the data now because we won't know which call
                    //  generated the data later.
                    //
    
                    NotCtx->BufferToFree = ExAllocatePoolWithTag( PagedPool,
                                                                  BufferSize,
                                                                  NC_TAG );
    
                    if (NotCtx->BufferToFree == NULL) {
    
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto NcPostNotifyDirectoryRealCleanup;
                    }
    
                }

                DestBuffer = NotCtx->BufferToFree;
                FLT_ASSERT( NotCtx->BufferLength == 0 );
            }
        }

        //
        //  If we have a zero length buffer, return to the user that something
        //  changed and do not attempt to return anything more.
        //

        if (BufferSize > 0) {

            Status = NcDirNotifyTranslateBuffers( InstanceContext,
                                                  NotCtx->IgnoreCase,
                                                  &NotCtx->UserRequestName,
                                                  (RequestContext->RequestType == NotifyMappingRequest)?
                                                      &NotCtx->MappingParentName:
                                                      &NotCtx->UserRequestName,
                                                  SourceBuffer,
                                                  DestBuffer,
                                                  SizeActuallyReturned,
                                                  BufferSize,
                                                  &InputConsumed,
                                                  &SizeWeReturn,
                                                  (BOOLEAN)(NotCtx->Mode == Munge || NotCtx->Mode == Merge),
                                                  (BOOLEAN)(RequestContext->RequestType == NotifyMappingRequest) );

        } else {

            FLT_ASSERT( NT_SUCCESS( Status ));
        }


        if (!NT_SUCCESS( Status )) {

            //
            //  If we failed, don't leave the garbage buffer to be picked
            //  up again.  Tear it down now.
            //

            if (DestBuffer == NotCtx->BufferToFree) {
                ExFreePoolWithTag( NotCtx->BufferToFree, NC_TAG );
                NotCtx->BufferToFree = NULL;
                NotCtx->BufferLength = 0;
            }
            goto NcPostNotifyDirectoryRealCleanup;
        }

        //
        //  We couldn't fit the translated information into the caller's
        //  buffer.  For directory change notifications, there is a special
        //  status to use in this situation, which tells the caller to
        //  rescan from scratch.  It's a heavy hammer, but might teach the
        //  caller to use a bigger buffer.
        //
        //  Unfortunately this is lost in translation between NT & Win32,
        //  so most applications end up guessing.
        //

        if ((BufferSize == 0) ||
            (InputConsumed < SizeActuallyReturned)) {

            if (!CompleteUserRequest) {

                ExFreePoolWithTag( NotCtx->BufferToFree, NC_TAG );
                NotCtx->BufferToFree = NULL;
                NotCtx->BufferLength = 0;
                NotCtx->InsufficientBufferSeen = TRUE;
            }

            SizeWeReturn = 0;
            Status = STATUS_NOTIFY_ENUM_DIR;
            goto NcPostNotifyDirectoryRealCleanup;

        } else {

            Status = STATUS_SUCCESS;

            if (DestBuffer == NotCtx->BufferToFree) {
                NotCtx->BufferLength = SizeWeReturn;
            }
        }

        //
        //  Our worst nightmare has come true.  The filesystem completed the
        //  request but we filtered out all the contents.  We certainly
        //  don't want to tell the caller nothing whatsoever happened, so we
        //  try to reissue this request.
        //  

        if (SizeWeReturn == 0) {

            PMDL OldMdl = Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.MdlAddress;
            PFLT_GENERIC_WORKITEM WorkItem;

            FLT_ASSERT( (NotCtx->Mode == Filter || NotCtx->Mode == Merge ) &&
                        (RequestContext->RequestType == NotifyShadowRequest || RequestContext->RequestType == NotifyMappingRequest ) );

            if (DestBuffer == NotCtx->BufferToFree) {

                ExFreePoolWithTag( NotCtx->BufferToFree, NC_TAG );
                NotCtx->BufferToFree = NULL;
                NotCtx->BufferLength = 0;
            }

            Status = NcGetDestinationNotifyBuffer( Data,
                                                   &DestBuffer,
                                                   &BufferSize );
            if (!NT_SUCCESS( Status )) {
                goto NcPostNotifyDirectoryRealCleanup;
            }

            WorkItem = FltAllocateGenericWorkItem();
            if (WorkItem == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto NcPostNotifyDirectoryRealCleanup;
            }

            FLT_ASSERT( RequestContext->Request == Data );

            //
            //  Clear and reinitialize the request using the Mdl and buffer
            //  that it was previously using.  Note that we know it will be
            //  system buffered if it has a buffer, because the only
            //  requests that can get here are requests we created.
            //

            FltReuseCallbackData( Data );

            Data->Iopb->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
            Data->Iopb->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
            Data->Iopb->OperationFlags = NotCtx->OperationFlags;
            Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Length =
                BufferSize;
            Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer =
                DestBuffer;
            Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.CompletionFilter =
                NotCtx->CompletionFilter;
            Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Spare1 = 0;
            Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.Spare2 = 0;
            Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.MdlAddress =
                OldMdl;

            if (BufferSize > 0) {

                Data->Flags |= FLTFL_CALLBACK_DATA_SYSTEM_BUFFER;
            }

            //
            //  Post issuing new IO to passive level, where we are sure TopLevelIrp
            //  is actually NULL.
            //

            Status = FltQueueGenericWorkItem( WorkItem,
                                              NcGlobalData.FilterHandle,
                                              NcReissueNotifyRequestWorkerRoutine,
                                              DelayedWorkQueue,
                                              RequestContext );

            if (NT_SUCCESS( Status )) {
                ReissuedRequest = TRUE;
                CompleteUserRequest = FALSE;
            } else {
                goto NcPostNotifyDirectoryRealCleanup;
            }

        }

        //
        //  If we successfully parsed and returned data either to the user's
        //  call or saved it to our buffer, we will not reissue the
        //  associated call.  If the user calls us again, we'll repeat any
        //  completed call(s).  Doing this allows the filter to delegate
        //  buffering of operations to the filesystem, except for the above
        //  case (both sides of a merge complete in parallel before the user
        //  calls again.)
        //


    } else {

        // 
        //  If the filesystem returned something less than one single
        //  entry, return nothing.
        //

        SizeWeReturn = 0;
        if (NotCtx->UserRequest == NULL) {
            CompleteUserRequest = FALSE;
        }
    }

NcPostNotifyDirectoryRealCleanup:

    FLT_ASSERT( CompleteUserRequest || RequestContext->RequestType != NotifyUserRequest );

    //
    //  We've just processed something from one of the child Irps.
    //  Tear it down, NULL it out.  We'll rebuild it if we're asked
    //  for another change notification.
    //

    if (!ReissuedRequest) {

        if (RequestContext->RequestType == NotifyShadowRequest ||
            RequestContext->RequestType == NotifyMappingRequest ) {

            if (Data == NotCtx->ShadowRequest) {
                NotCtx->ShadowRequest = NULL;
            }

            if (Data == NotCtx->MappingRequest) {
                NotCtx->MappingRequest = NULL;
            }

            //
            //  This routine will free the FLT_CALLBACK_DATA.
            //
            
            NcCleanupSubNotifyRequest( Data );
        }
    }

    //
    //  We need to write back results of query.
    //

    if (CompleteUserRequest) {

        FLT_ASSERT( UnlockContext );

        FLT_ASSERT( NotCtx->UserRequest != NULL );
        FLT_ASSERT( RequestContext->RequestType != NotifyUserRequest ||
                    Data == NotCtx->UserRequest );

        NotCtx->UserRequest->IoStatus.Status = Status;

        if (NT_SUCCESS( Status )) {
 
            //success
            NotCtx->UserRequest->IoStatus.Information = SizeWeReturn;

        } else {
 
            //failure
            NotCtx->UserRequest->IoStatus.Information = 0;
        }

        if (RequestContext->RequestType != NotifyUserRequest) {

            PFLT_CALLBACK_DATA RequestToComplete = NotCtx->UserRequest;
            NTSTATUS CancelStatus;

            //
            //  Clear the cancel routine.  If cancel has already been
            //  invoked, don't complete now (leave the cancel routine to
            //  complete the request, and give the user nothing.)  If
            //  cancel has not yet been invoked, after we clear the
            //  routine it cannot be invoked, so we are free to complete
            //  the request ourselves unconditionally.
            //

            CancelStatus = FltClearCancelCompletion( RequestToComplete );

            if (CancelStatus != STATUS_CANCELLED) {

                NotCtx->UserRequest = NULL;

                NcUnlockStreamHandleContext( HandleContext );
                UnlockContext = FALSE;

                FltCompletePendedPreOperation( RequestToComplete,
                                               FLT_PREOP_COMPLETE,
                                               NULL );
            }

        } else {

            NotCtx->UserRequest = NULL;
        }
    }

    if (SourceBuffer != NULL) {

        ExFreePoolWithTag( SourceBuffer, NC_TAG );
    }
 
    if (UnlockContext) {
 
        FLT_ASSERT( HandleContext != NULL );
        NcUnlockStreamHandleContext( HandleContext );
    }

    //
    //  Now that we've dropped the lock, drop our references.
    //

    if (!ReissuedRequest) {
        NcFreeNotifyRequestContext( RequestContext );
    }
 
}

FLT_POSTOP_CALLBACK_STATUS
NcPostNotifyDirectorySafe (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    Routine is invoked when a directory change notification request
    completes.  The request must be the user's request.  This
    function is called when <= APC_LEVEL.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.  For directory change notifications, this is a pointer
        to our handle context.

    Flags - Flags for this operation.

Return Value:

    The return value is the Status of the operation.

--*/
{

    PAGED_CODE();

    NcPostNotifyDirectoryReal( Data, CompletionContext );

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( FltObjects );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_POSTOP_CALLBACK_STATUS
NcPostNotifyDirectory (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    Routine is invoked when a directory change notification request
    completes.  The request must be the user's request.  This routine
    may be called at DISPATCH_LEVEL and must be nonpaged.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.  For directory change notifications, this is a pointer
        to our handle context.

    Flags - Flags for this operation.

Return Value:

    The return value is the Status of the operation.

--*/
{

    FLT_POSTOP_CALLBACK_STATUS Status;
    BOOLEAN Success;

    if (FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING )) {

        //
        //  We can only be here on a user initiated operation that we sent
        //  to the filesystem.
        //
        //  TODO: Really what we want to do here is return
        //  STATUS_NOTIFY_ENUM_DIR to the caller, but we no longer own the
        //  request.  Should we bite the bullet and shadow all munge requests
        //  so that this can work?
        //

        NcFreeNotifyRequestContext( CompletionContext );

        Status = FLT_POSTOP_FINISHED_PROCESSING;

    } else {

        //
        //  For simplicity, and to avoid needing everything to be nonpaged,
        //  we perform completion at APC_LEVEL or below.
        //

        Success = FltDoCompletionProcessingWhenSafe( Data,
                                                     FltObjects,
                                                     CompletionContext,
                                                     Flags,
                                                     NcPostNotifyDirectorySafe,
                                                     &Status );

        if (!Success) {

            //
            //  It would be nice to know _why_ this fails, but since we
            //  don't, neither will anybody else.
            //

            Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
            Data->IoStatus.Information = 0;
            Status = FLT_POSTOP_FINISHED_PROCESSING;
        }
    }

    return Status;
}

VOID
NcReissueNotifyRequestWorkerRoutine (
    _In_ PFLT_GENERIC_WORKITEM WorkItem,
    _In_ PFLT_FILTER Filter,
    _In_ PVOID RequestPtr
    )
/*++

Routine Description:

    This function reissues an subrequest that is part of an
    oustanding directory change notification request.  If it is
    unable to reissue the request, this function is responsible
    for all cleanup of the request.

Arguments:

    WorkItem - Pointer to the workitem which triggered the call to this
        function.

    Filter - Pointer to our filter object.

    RequestPtr - Our context, specifically the request we wish to reissue.

Return Value:

    None.

--*/
{
    PNC_NOTIFY_REQUEST_CONTEXT RequestContext = (PNC_NOTIFY_REQUEST_CONTEXT)RequestPtr;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Filter );

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    FLT_ASSERT( RequestContext->RequestType == NotifyShadowRequest ||
                RequestContext->RequestType == NotifyMappingRequest );

    //
    //  Since we're being called asynchronously, it's possible
    //  that this request has already been cancelled, or that
    //  cleanup has already occurred.  In the event of a cancel,
    //  this request should be marked as canceled and the
    //  filesystem will complete it immediately.  In the event
    //  of cleanup, we are sending the request to a cleaned up
    //  handle and the filesystem will complete it immediately.
    //
    //  On failure, Fltmgr will call our completion routine,
    //  so we don't need to clean up here.
    //

    (VOID) FltPerformAsynchronousIo( RequestContext->Request,
                                     NcPostNotifyDirectoryReal,
                                     RequestContext );

    FltFreeGenericWorkItem( WorkItem );
}

VOID
NcCloseHandleWorkerRoutine (
    _In_ PFLT_GENERIC_WORKITEM WorkItem,
    _In_ PFLT_FILTER Filter,
    _In_ PVOID HandlePtr
    )
/*++

Routine Description:

    This function performs a handle close operation.  Since handle
    close operations must occur at passive, we queue a workitem to
    close the handle when we cannot do so inline.  This function
    actually performs the close.

Arguments:

    WorkItem - Pointer to the workitem which triggered the call to this
        function.

    Filter - Pointer to our filter object.

    HandlePtr - Our context, specifically the handle we wish to close.

Return Value:

    None.

--*/
{
    HANDLE Handle = (HANDLE)HandlePtr;
    NTSTATUS DummyStatus;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Filter );

    DummyStatus = FltClose( Handle );
    FLT_ASSERT( NT_SUCCESS( DummyStatus ));

    FltFreeGenericWorkItem( WorkItem );
}

_Pre_satisfies_(*LockHeld)
_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(*HandleContext->Lock)
_When_(*LockHeld == FALSE, _Releases_lock_(_Global_critical_region_))
_When_(*LockHeld == FALSE, _Releases_lock_(*HandleContext->Lock))
VOID
NcNotifyAbort (
    _In_ PNC_STREAM_HANDLE_CONTEXT HandleContext,
    _In_ NTSTATUS Status,
    _Inout_ PBOOLEAN LockHeld
    )
/*++

Routine Description:

    This function aborts a change notify operation.  It is shared code
    called from both cancel and handle cleanup.  It assumes that it is
    called with the context lock already held, and may drop the lock
    if it needs to complete or cancel requests.

Arguments:

    HandleContext - The context describing the request we wish to abort.

    Status - Status code to abort with.  This should be STATUS_CANCELLED
        for cancel requests or STATUS_NOTIFY_CLEANUP for cleanup requests.

    LockHeld - Pointer to a boolean value.  On input, this should contain
        a value indicating whether the handle context lock is acquired
        (and it should be.)  On output, this will be set to TRUE if the
        lock is acquired or FALSE if not.

Return Value:

    None.

--*/
{
    PNC_DIR_NOT_CONTEXT NotCtx = &HandleContext->DirectoryNotificationContext;

    FLT_ASSERT( *LockHeld );

    PAGED_CODE();

    //
    //  If we are hanging on to the user's request, complete it now.  The status
    //  code for this is very specific.  Note that in the Munge case the request
    //  is owned by the filesystem, so we leave the filesystem to deal with this
    //  when it gets the cleanup request.
    //
    //  Cancel any outstanding requests we have issued (if any.)  
    //

    if ((NotCtx->Mode == Filter) ||
        (NotCtx->Mode == Merge)) {

        PFLT_CALLBACK_DATA RequestToComplete = NotCtx->UserRequest;
        PFLT_CALLBACK_DATA RequestToCancel1 = NotCtx->ShadowRequest;
        PFLT_CALLBACK_DATA RequestToCancel2 = NotCtx->MappingRequest;
        HANDLE HandleToClose = NotCtx->RealParentHandle;
        PFILE_OBJECT FileObjectToDereference = NotCtx->RealParentFileObject;
        PFLT_GENERIC_WORKITEM WorkItem = NotCtx->RealParentCloseWorkItem;

        NotCtx->ShadowRequest = NULL;
        NotCtx->MappingRequest = NULL;
        NotCtx->RealParentHandle = NULL;
        NotCtx->RealParentFileObject = NULL;
        NotCtx->RealParentCloseWorkItem = NULL;

        //
        //  Attempt to clear the cancel callback on the user's request.  If
        //  this fails, it may indicate that we are cancelling the irp ourselves,
        //  or may indicate that we're in cleanup and racing with a cancel.
        //  When racing with cancel, cancel always wins.
        //

        if (RequestToComplete) {
            NTSTATUS CancelStatus;

            CancelStatus = FltClearCancelCompletion( RequestToComplete );

            if (CancelStatus == STATUS_CANCELLED && Status != STATUS_CANCELLED) {
                RequestToComplete = NULL;
            } else {
                NotCtx->UserRequest = NULL;
            }
        }

        NcUnlockStreamHandleContext( HandleContext );
        *LockHeld = FALSE;

        if (RequestToComplete) {
            RequestToComplete->IoStatus.Status = Status;
            RequestToComplete->IoStatus.Information = 0;

            FltCompletePendedPreOperation( RequestToComplete, FLT_PREOP_COMPLETE, NULL );
        }


        //
        //  Cancel any outstanding IOs, ignoring status.
        //
        //  It is possible that we can't cancel because:
        //
        //  1. The request hasn't reached the filesystem so there's no cancel
        //     routine.  This is not a problem, since the below call will
        //     mark the request as cancelled so it will complete immediately.
        //
        //  2. The request is already being completed.  It's no big deal if
        //     we let it complete, just so long as it finishes soon.  Note
        //     that we've already set CleanupSeen/CancelSeen by this point,
        //     so unless we're cancelling and get a new user request, the
        //     outcome will be the same as if our cancel succeeded.
        //

        if (RequestToCancel1) {

            (VOID)FltCancelIo( RequestToCancel1 );
        }

        if (RequestToCancel2) {

            (VOID)FltCancelIo( RequestToCancel2 );
        }

        if (HandleToClose) {

            NTSTATUS WorkItemStatus;

            FLT_ASSERT( WorkItem != NULL );

            WorkItemStatus = FltQueueGenericWorkItem( WorkItem,
                                                      NcGlobalData.FilterHandle,
                                                      NcCloseHandleWorkerRoutine,
                                                      DelayedWorkQueue,
                                                      HandleToClose );

            //
            //  Queuing the work item will fail if the filter is unloading.  All
            //  we're doing is a handle close, which will get taken care of when
            //  the process we're in eventually goes away.  However we've still
            //  got an allocated work item to take care of.
            //

            if (!NT_SUCCESS( WorkItemStatus )) {

                FltFreeGenericWorkItem( WorkItem );
            }
        }

        if (FileObjectToDereference) {
            ObDereferenceObject( FileObjectToDereference );
        }
    }
}

VOID
NcNotifyCancelCallback(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This function is called to cancel a change notify operation.

Arguments:

    Data - The request to cancel.  This should be caller-initiated (or we
        shouldn't have set a cancel routine on it.)

Return Value:

    None.

--*/
{
    PNC_STREAM_HANDLE_CONTEXT HandleContext;
    PNC_DIR_NOT_CONTEXT NotCtx;
    NTSTATUS Status;
    BOOLEAN UnlockContext;

    PAGED_CODE();

    //
    //  Try to figure out the context associated with this request.
    //  It would be really nice if we had a FltObjects structure for
    //  this.  Are filters really supposed to do this?
    //

    Status = FltGetStreamHandleContext( Data->Iopb->TargetInstance,
                                        Data->Iopb->TargetFileObject,
                                        &HandleContext );

    FLT_ASSERT( NT_SUCCESS( Status ));
    if (!NT_SUCCESS( Status )) {
        return;
    }

    FLT_ASSERT( HandleContext != NULL );

    NotCtx = &HandleContext->DirectoryNotificationContext;

    //
    //  Before looking at the context, we have to acquire the lock.
    //
    
    NcLockStreamHandleContext( HandleContext );
    UnlockContext = TRUE;

    //
    //  We should only ever be called on the user's request.
    //  It follows that this is a Merge or Filter operation.
    //
    
    FLT_ASSERT( Data == NotCtx->UserRequest );
    FLT_ASSERT( NotCtx->Mode == Filter || NotCtx->Mode == Merge );

    //
    //  Indicate to completing requests that we are cancelling.
    //  While we are cancelling, if child requests are racing
    //  with us and completing legitimately, we force them to
    //  drain out.  This flag will be cleared next time the
    //  user asks for a change notification on this handle.
    //

    NotCtx->CancelSeen = TRUE;

    //
    //  If the user is cancelling, it follows that they cannot
    //  expect a complete picture of data.  Tear down any held
    //  over buffer that we may happen to have, since it may be
    //  large and we're not optimizing for recurring work after
    //  cancel.
    //
    //  We cannot tear down any file names at this point, since
    //  we're racing with IO completion which may depend on
    //  those names remaining intact.
    //

    if (NotCtx->BufferToFree != NULL) {
    
        ExFreePoolWithTag( NotCtx->BufferToFree, NC_TAG );
        NotCtx->BufferToFree = NULL;
        NotCtx->BufferLength = 0;
    }

    //
    //  This call may drop our locks.  If it does, we are no
    //  longer synchronized against incoming requests and
    //  must not tear anything down any further.
    //

    NcNotifyAbort( HandleContext, STATUS_CANCELLED, &UnlockContext );

    if (UnlockContext) {
        NcUnlockStreamHandleContext( HandleContext );
        UnlockContext = FALSE;
    }

    FltReleaseContext( HandleContext );
}

NTSTATUS 
NcStreamHandleContextNotCreate( 
    _Out_ PNC_DIR_NOT_CONTEXT Context
    )
/*++

Routine Description:

    This function is called to initialize the directory change notification
    portion of a stream handle context.

Arguments:

    Context - Pointer to the directory change notification portion of the
        stream handle context.

Return Value:

    Returns the status of the operation (currently only STATUS_SUCCESS.)

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Context->Mode = Uninitialized;
    Context->CleanupSeen = FALSE;
    Context->InsufficientBufferSeen = FALSE;
    Context->UserRequestName.Buffer = NULL;
    Context->UserRequestName.Length = Context->UserRequestName.MaximumLength = 0;

    Context->MappingParentName.Buffer = NULL;
    Context->MappingParentName.Length = Context->MappingParentName.MaximumLength = 0;

    Context->UserRequest = NULL;
    Context->ShadowRequest = NULL;
    Context->MappingRequest = NULL;

    Context->InstanceContext = NULL;
    Context->RealParentHandle = NULL;
    Context->RealParentFileObject = NULL;

    Context->BufferToFree = NULL;
    Context->BufferLength = 0;

    return Status;
}

VOID 
NcStreamHandleContextNotCleanup(
    _In_ PNC_STREAM_HANDLE_CONTEXT HandleContext 
    )
/*++

Routine Description:

    This function is called to process IRP_MJ_CLEANUP for a directory change
    notification request.  Directory change notifications are cancelled
    with a special status code at handle cleanup time.  We take this
    opportunity to tear down anything we reasonably can, since a file may
    be in limbo between cleanup and close for a long time.

Arguments:

    HandleContext - Pointer to the stream handle context.

Return Value:

    None.

--*/
{
    PNC_DIR_NOT_CONTEXT NotCtx = &HandleContext->DirectoryNotificationContext;
    BOOLEAN UnlockContext;

    PAGED_CODE();

    NcLockStreamHandleContext( HandleContext );
    UnlockContext = TRUE;

    //
    //  Set a flag to indicate that outstanding requests should complete immediately
    //  without propagating status to the user or buffering results.  This also
    //  prevents new work starting after this point.
    //

    FLT_ASSERT( !NotCtx->CleanupSeen );
    NotCtx->CleanupSeen = TRUE;

    //
    //  Complete our requests.  Note that this routine may drop our lock.
    //

    NcNotifyAbort( HandleContext, STATUS_NOTIFY_CLEANUP, &UnlockContext );

    //
    //  Lock the structure again if necessary after performing any IO so we can
    //  continue teardown.  Note that the user's request should not come back
    //  at this point in the case of cleanup.
    //

    if (!UnlockContext) {
        NcLockStreamHandleContext( HandleContext );
        UnlockContext = TRUE;
    }

    //
    //  Once a handle is closed, it should stay closed.
    //

    FLT_ASSERT( NotCtx->CleanupSeen );

    FLT_ASSERT( NotCtx->UserRequest == NULL ||
                NotCtx->Mode == Munge);

    //
    //  Although requests may remain, if we've been through cleanup our post
    //  routine will ensure they can't do squat.
    //
    
    if (NotCtx->UserRequestName.Buffer != NULL) {
        NcFreeUnicodeString( &NotCtx->UserRequestName );
    }
    
    if (NotCtx->MappingParentName.Buffer != NULL) {
        NcFreeUnicodeString( &NotCtx->MappingParentName );
    }
    
    if (NotCtx->BufferToFree != NULL) {
    
        ExFreePoolWithTag( NotCtx->BufferToFree, NC_TAG );
        NotCtx->BufferToFree = NULL;
        NotCtx->BufferLength = 0;
    }

    if (NotCtx->InstanceContext != NULL) {
        FltReleaseContext( NotCtx->InstanceContext );
        NotCtx->InstanceContext = NULL;
    }
    
    NotCtx->Mode = Uninitialized;

    NcUnlockStreamHandleContext( HandleContext );
    UnlockContext = FALSE;
}

VOID 
NcStreamHandleContextNotClose(
    _In_ PNC_DIR_NOT_CONTEXT Context 
    )
/*++

Routine Description:

    This function is called to tear down the directory change notification
    portion of a stream handle context.

Arguments:

    Context - Pointer to the directory change notification portion of the
        stream handle context.

Return Value:

    None.

--*/
{

    PAGED_CODE();

    //
    //  TODO: Determine when/how this is called.  What IRQL are we really at
    //  here?  Are we top level?  What should we do if we're not?
    //
    FLT_ASSERT( IoGetTopLevelIrp() == NULL );
    FLT_ASSERT( !KeAreAllApcsDisabled() );


    //
    //  We need to be protected against any new work arriving on this handle.
    //  Typically we expect cleanup to have already been seen by this point,
    //  which will have torn down most state for this handle.  This is not
    //  necessarily true when the filter is being detached.  Since we can't
    //  tear down a context with active references, we expect that mapping/
    //  shadow requests are currently NULL, and should not come back - either
    //  because CleanupSeen is set, or FltMgr is draining us.
    //

    FLT_ASSERT( Context->MappingRequest == NULL &&
                Context->ShadowRequest == NULL );

    //
    //  Firstly, close the handles.  These can recurse back into the
    //  filesystem and generate notifications themselves, or cancel
    //  outstanding notifications, etc.  Note that we do not hold the lock
    //  around this operation.
    //

    if (Context->RealParentHandle != NULL) {

        FltClose( Context->RealParentHandle );

    }

    //
    //  In theory, we should no longer have outstanding notifications at
    //  this point.  The user has closed their handle (completing any
    //  shadow request) and we have closed ours (completing any mapping
    //  request.)  Now we are safe to teardown, and we don't require a
    //  lock to do so.
    //

    if (Context->RealParentFileObject != NULL) {

        ObDereferenceObject( Context->RealParentFileObject );
    }

    Context->RealParentFileObject = NULL;
    Context->RealParentHandle = NULL;

    if (Context->RealParentCloseWorkItem != NULL) {
        FltFreeGenericWorkItem( Context->RealParentCloseWorkItem );
        Context->RealParentCloseWorkItem = NULL;
    }

    if (Context->UserRequestName.Buffer != NULL) {
        NcFreeUnicodeString( &Context->UserRequestName );
    }

    if (Context->MappingParentName.Buffer != NULL) {
        NcFreeUnicodeString( &Context->MappingParentName );
    }

    if (Context->BufferToFree != NULL) {

        ExFreePoolWithTag( Context->BufferToFree, NC_TAG );
        Context->BufferToFree = NULL;
        Context->BufferLength = 0;
    }

    Context->Mode = Uninitialized;

    if (Context->InstanceContext != NULL) {
        FltReleaseContext( Context->InstanceContext );
        Context->InstanceContext = NULL;
    }
}



