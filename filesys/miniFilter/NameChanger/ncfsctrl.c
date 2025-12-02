/*++

Copyright (c) 2008 - 2009  Microsoft Corporation

Module Name:

    ncfsctrl.c

Abstract:

    Contains routines to process user-initiated file system control
    (FSCTL) requests.

Environment:

    Kernel mode

--*/

#include "nc.h"

NTSTATUS
NcFindFilesBySidTranslateBuffers (
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ BOOLEAN IgnoreCase,
    _In_ PUNICODE_STRING UserRequestName,
    _In_ PUNICODE_STRING OpenedName,
    _In_ PFILE_NAME_INFORMATION InputSystemBuffer,
    _Out_writes_bytes_to_(OutputBufferLength, *OutputBufferWritten) PFILE_NAME_INFORMATION OutputUserBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG InputBufferConsumed,
    _Out_ PULONG OutputBufferWritten,
    _In_ BOOLEAN ReturnRealMappingPaths
    ) ;

NTSTATUS
NcUsnTranslateBuffers (
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ BOOLEAN IgnoreCase,
    _In_ ULONGLONG RealMappingParentId,
    _In_ ULONGLONG UserMappingParentId,
    _In_ PUSN_RECORD InputSystemBuffer,
    _Out_writes_bytes_to_(OutputBufferLength, *OutputBufferWritten) PUSN_RECORD OutputUserBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG InputBufferConsumed,
    _Out_ PULONG OutputBufferWritten
    );

VOID
NcPostReadUsnJournalWorker (
    _In_ PFLT_GENERIC_WORKITEM WorkItem,
    _In_ PFLT_FILTER Filter,
    _In_ PVOID Context
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcFindFilesBySidTranslateBuffers)
#pragma alloc_text(PAGE, NcStreamHandleContextFindBySidCreate)
#pragma alloc_text(PAGE, NcStreamHandleContextFindBySidClose)
#pragma alloc_text(PAGE, NcPreFindFilesBySid)
#pragma alloc_text(PAGE, NcPostFindFilesBySid)
#if FLT_MGR_WIN7
#pragma alloc_text(PAGE, NcPostLookupStreamFromCluster)
#endif
#pragma alloc_text(PAGE, NcUsnTranslateBuffers)
#pragma alloc_text(PAGE, NcPostEnumUsnData)
#pragma alloc_text(PAGE, NcPostReadUsnJournalWorker)
#pragma alloc_text(PAGE, NcPostReadFileUsnData)
#endif

NTSTATUS
NcStreamHandleContextFindBySidCreate (
    _Out_ PNC_FIND_BY_SID_CONTEXT Context
    )
/*++

Routine Description:

    This function is called to initialize the find files by SID portion of
    a stream handle context.

Arguments:

    Context - Pointer to the find files by SID portion of the stream handle
        context.

Return Value:

    Returns the status of the operation (currently only STATUS_SUCCESS.)

--*/
{

    PAGED_CODE();

    Context->RealHandle = NULL;
    Context->RealFileObject = NULL;

    Context->BufferToFree = NULL;
    Context->BufferSize = 0;
    Context->CurrentEntry = 0;

    Context->OutstandingRequests = 0;

    return STATUS_SUCCESS;

}

VOID
NcStreamHandleContextFindBySidClose (
    _In_ PNC_FIND_BY_SID_CONTEXT Context
    )
/*++

Routine Description:

    This function is called to tear down the find files by SID portion of
    a stream handle context.

Arguments:

    Context - Pointer to the find files by SID portion of the stream handle
        context.

Return Value:

    None.

--*/
{

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  This assert is overactive.  There are cases which we cannot
    //  prevent which can cause it to fire (eg. we successfully
    //  sent a request down but failed on completion before being able
    //  to obtain/lock the handle context.)  The effect of leaving
    //  this nonzero is that we will not clean up the handle and
    //  file object until the user's handle is closed.  We are prepared
    //  to live with that (and the user could generate this condition
    //  herself by ceasing issuing queries before the query is fully
    //  complete.)
    //

    FLT_ASSERT( Context->OutstandingRequests == 0 );

    if (Context->RealHandle != NULL) {

        FltClose( Context->RealHandle );
        Context->RealHandle = NULL;
    }

    if (Context->RealFileObject != NULL) {

        ObDereferenceObject( Context->RealFileObject );
        Context->RealFileObject = NULL;
    }

    if (Context->BufferToFree != NULL) {

        ExFreePoolWithTag( Context->BufferToFree, NC_TAG );
        Context->BufferToFree = NULL;
        Context->BufferSize = 0;
        Context->CurrentEntry = 0;
    }

    FLT_ASSERT( Context->BufferSize == 0 && Context->CurrentEntry == 0 );

}

NTSTATUS
NcFindFilesBySidTranslateBuffers (
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ BOOLEAN IgnoreCase,
    _In_ PUNICODE_STRING UserRequestName,
    _In_ PUNICODE_STRING OpenedName,
    _In_ PFILE_NAME_INFORMATION InputSystemBuffer,
    _Out_writes_bytes_to_(OutputBufferLength, *OutputBufferWritten) PFILE_NAME_INFORMATION OutputUserBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG InputBufferConsumed,
    _Out_ PULONG OutputBufferWritten,
    _In_ BOOLEAN ReturnRealMappingPaths
    )
/*++

Routine Description:

    This routine is used to transform buffers from a filesystem view to the
    user view.  Depending on flags we either replace references to the real
    mapping with the user mapping or suppress all entries from the real
    mapping.

Arguments:

    InstanceContext - Pointer to the context describing this instance of the
        filter.

    IgnoreCase - TRUE if comparisons should be case insensitive, FALSE if
        comparisons should be case sensitive.

    UserRequestName - The string for the path that the user opened and is
        finding child files by SID on.

    OpenedName - The string for the path that we are processing child files
        on.  "Typically" the same as UserRequestName, but may be different if
        we are merging results from the user's handle with those of the
        real mapping.  In this case, OpenedName may refer to the path to
        the real mapping.

    InputSystemBuffer - The buffer we are processing from.  Note that this
        routine assumes the buffer is not volatile (cannot be externally
        modified.)  For this reason, the buffer is expected to be system
        buffered by the caller if it is not already.

        Note however that the contents of the buffer may have originated from
        a user buffer (e.g. via a memcpy), so although the contents are non-volatile,
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

Return Value:

    The return value is the Status of the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NC_PATH_OVERLAP RealOverlap;

    //
    //  Pointers into the above buffers used as we read entries from
    //  one buffer and write them to the other.
    //

    PFILE_NAME_INFORMATION SourceEntry = NULL;
    PFILE_NAME_INFORMATION DestEntry = NULL;

    //
    //  The name of the object opened by the user; a full path to the path
    //  returned from the filesystem that we are currently examining; the
    //  Remainder of that path if it turns out to be in a mapping; the
    //  transformed name if one is required.
    //

    UNICODE_STRING NameString = EMPTY_UNICODE_STRING;
    UNICODE_STRING Remainder;
    UNICODE_STRING MungedName = EMPTY_UNICODE_STRING;

    //
    //  A pointer to one of the above buffers that we intend on returning
    //  to the application.  Note that this may be NULL if an entry is
    //  being suppressed.
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

        while( SourceEntry ) {

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
                goto NcFindFilesBySidTranslateBuffersCleanup;
            }

            Status = RtlULongAdd( EntryLength,
                                  SourceEntry->FileNameLength,
                                  &EntryLength );

            if (!NT_SUCCESS( Status ) ||
                (EntryLength >= MAXUSHORT)) {

                Status = STATUS_OBJECT_PATH_INVALID;
                goto NcFindFilesBySidTranslateBuffersCleanup;
            }

            Status = RtlULongAdd( *InputBufferConsumed,
                                  FIELD_OFFSET( FILE_NAME_INFORMATION, FileName ),
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
                //  system buffered, we cannot guarantee that a caller is not
                //  corrupting it in an in-flight request.  This condition
                //  should never occur without aforementioned corruption.
                //

                FLT_ASSERT( FALSE );
                Status = STATUS_INVALID_USER_BUFFER;
                goto NcFindFilesBySidTranslateBuffersCleanup;

            }

            if (EntryLength > NameString.MaximumLength) {

                if (NameString.Buffer != NULL) {
                    NcFreeUnicodeString( &NameString );
                }

                NameString.Buffer = ExAllocatePoolZero( PagedPool,
                                                        EntryLength,
                                                        NC_TAG );

                if (NameString.Buffer == NULL) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto NcFindFilesBySidTranslateBuffersCleanup;
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
                //  ancestor of both real and user mappings.  In this case, we
                //  need to translate real mapping paths to user mapping paths.
                //

                Status = NcConstructPath( &InstanceContext->Mapping.UserMapping,
                                          &Remainder,
                                          TRUE,
                                          &MungedName );

                if (!NT_SUCCESS( Status )) {
                    goto NcFindFilesBySidTranslateBuffersCleanup;
                }

                ReturnName = &MungedName;

            } else if (RealOverlap.InMapping) {

                //
                //  If the name is in the real mapping but the caller is asking
                //  for a tree which is not an ancestor of the user mapping,
                //  then this name should not be visible under this tree.
                //

                ReturnName = NULL;

            } else {

                ReturnName = &NameString;

            }

            if (ReturnName != NULL) {

                //
                //  Since this API returns paths under the one the query was
                //  issued on, the path we have now had better be longer than
                //  that path length.  Even if we refer to the same object,
                //  our path includes a trailing slash.
                //

                FLT_ASSERT( ReturnName->Length > UserRequestName->Length );

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

                EntryLength = AlignToSize( FIELD_OFFSET( FILE_NAME_INFORMATION, FileName ) + ReturnName->Length - UserRequestName->Length - sizeof(WCHAR), 8);

                //
                //  We've done all we can.  Return now to let our caller deal
                //  with the remaining buffer.
                //

                Status = RtlULongAdd( EntryLength,
                                      *OutputBufferWritten,
                                      &UlongResult );

                if (!NT_SUCCESS( Status ) ||
                    (UlongResult > OutputBufferLength)) {

                    SourceEntry = NULL;
                    DestEntry = NULL;
                    break;
                }

                //
                //  Copy the relative path name, taking care to exclude the
                //  initial slash.
                //

                DestEntry->FileNameLength = ReturnName->Length - UserRequestName->Length - sizeof(WCHAR);
                RtlCopyMemory( DestEntry->FileName,
                               Add2Ptr( ReturnName->Buffer, UserRequestName->Length + sizeof(WCHAR)),
                               ReturnName->Length - UserRequestName->Length - sizeof(WCHAR));

                //
                //  Advance the destination that we're writing new entries by
                //  however much we just consumed.
                //

                *OutputBufferWritten += EntryLength;
                DestEntry = Add2Ptr( DestEntry, EntryLength );

                if (MungedName.Buffer != NULL) {
                    ExFreePoolWithTag( MungedName.Buffer, NC_GENERATE_NAME_TAG );
                    MungedName.Buffer = NULL;
                }
            }

            //
            //  Now calculate and advance the location we're reading and
            //  processing from.  If we're have no more buffer left, we're
            //  done.
            //

            EntryLength = AlignToSize( FIELD_OFFSET( FILE_NAME_INFORMATION, FileName ) + SourceEntry->FileNameLength, 8 );

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
                (PointerResult < Add2Ptr( InputSystemBuffer, sizeof(FILE_NAME_INFORMATION) ))) {

                FLT_ASSERT( FALSE );

                Status = STATUS_INVALID_USER_BUFFER;
                goto NcFindFilesBySidTranslateBuffersCleanup;
            }

            SourceEntry = (PFILE_NAME_INFORMATION)PointerResult;

            //
            //  If we've just advanced our next location beyond the end of the
            //  input buffer, or there isn't enough room in it for even a FILE_NAME_INFORMATION
            //  structure, terminate the loop by setting SourceEntry to NULL so
            //  we can at least return the valid entries we have.
            //

            FLT_ASSERT( *InputBufferConsumed <= InputBufferLength );

            if ((*InputBufferConsumed >= InputBufferLength) ||
                (*InputBufferConsumed + FIELD_OFFSET( FILE_NAME_INFORMATION, FileName ) > InputBufferLength)) {

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

NcFindFilesBySidTranslateBuffersCleanup:

    if (MungedName.Buffer != NULL) {

        ExFreePoolWithTag( MungedName.Buffer, NC_GENERATE_NAME_TAG );
    }

    if (NameString.Buffer != NULL) {

        NcFreeUnicodeString( &NameString );
    }

    return Status;

}

FLT_PREOP_CALLBACK_STATUS
NcPreFindFilesBySid (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Routine is invoked when the user wants to query files owned by
    a particular SID under a subtree.  For this call, we must return
    paths relative to the user's handle.

    Implementationally, we have four cases:

    1. If the handle is an ancestor or match of the real mapping, we may
       need to remove entries.

    2. If the handle is an ancestor or match of the user mapping, we may
       need to insert entries.  We implement this in two phases: first,
       we enumerate all results under the user's handle; when complete,
       we open a handle to the mapping and enumerate all results under
       that.

    3. If the handle is an ancestor of both, we may need to transform
       entries.

    4. If the handle is not an ancestor of either, we have no work to do.

    For cases 2 and 3, the size of the data may change.  Since we can't
    omit entries, we add those to a buffer, then return (and process)
    entries from the buffer next time this function is invoked on the
    handle.  Only when we have nothing buffered do we continue sending
    requests to the filesystem.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure
        containing opaque handles to this filter, instance, its
        associated volume and file object.

    CompletionContext - The context for the completion routine for
        this operation.  We set this to the file object we are enumerating
        under for find files by SID.

Return Value:

    The return value is the Status of the operation.

--*/
{
    NTSTATUS Status;
    FLT_PREOP_CALLBACK_STATUS ReturnValue = FLT_PREOP_SYNCHRONIZE;
    NC_PATH_OVERLAP RealOverlap, UserOverlap;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    PFLT_FILE_NAME_INFORMATION FileInfoInternalHandle = NULL;
    PNC_STREAM_HANDLE_CONTEXT HandleContext = NULL;
    PNC_FIND_BY_SID_CONTEXT FindBySidCtx = NULL;
    ULONG SizeWeReturn = 0;
    BOOLEAN UnlockContext = FALSE;

    //
    //  The name of the object opened by the user, and the path of the object
    //  we're issuing queries on.  These are typically the same, but will
    //  differ when we're injecting entries (user opens an ancestor of the
    //  user mapping, but we're sending queries to the real mapping.)
    //

    UNICODE_STRING UserRequestName;
    UNICODE_STRING OpenedName;

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    if (Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength < sizeof( FILE_NAME_INFORMATION )) {

        //
        //  This somewhat strange code is used to maintain parity with the
        //  filesystem.
        //

        ReturnValue = FLT_PREOP_COMPLETE;
        Status = STATUS_INVALID_USER_BUFFER;
        goto NcPreFindFilesBySidCleanup;
    }

    //
    //  Get our instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreFindFilesBySidCleanup;
    }

    //
    //  Get the file's name.
    //

    Status = NcGetFileNameInformation( Data,
                                       NULL,
                                       NULL,
                                       FLT_FILE_NAME_OPENED |
                                           FLT_FILE_NAME_QUERY_DEFAULT |
                                           FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                       &FileInfo );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreFindFilesBySidCleanup;
    }

    Status = FltParseFileNameInformation( FileInfo );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreFindFilesBySidCleanup;
    }

    //
    //  If the path that was used for the query ends in a trailing slash
    //  (ie., the root directory), shave it off here.  This keeps our logic
    //  simple, rather than dealing with this over and over.
    //

    UserRequestName = FileInfo->Name;
    FLT_ASSERT( UserRequestName.Length > 0 );
    if (UserRequestName.Buffer[UserRequestName.Length/sizeof(WCHAR) - 1] == NC_SEPARATOR) {
        UserRequestName.Length -= sizeof(WCHAR);
    }

    //
    //  As an optimization, check if the handle queried on is an ancestor of
    //  either mapping.  Cases to consider:
    //
    //  1. If the handle is an ancestor or match of the real mapping, we may
    //     need to remove entries.
    //
    //  2. If the handle is an ancestor or match of the user mapping, we may
    //     need to insert entries.
    //
    //  3. If the handle is an ancestor of both, we may need to transform
    //     entries.
    //
    //  4. If the handle is not an ancestor of either, we have no work to
    //     do.  As an optimization, we can stop now if this is the case.
    //

    NcComparePath( &FileInfo->Name,
                   &InstanceContext->Mapping.UserMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &UserOverlap );

    NcComparePath( &FileInfo->Name,
                   &InstanceContext->Mapping.RealMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &RealOverlap );

    if (!(UserOverlap.Ancestor || RealOverlap.Ancestor)) {

        //
        //  In case 4 above, leave now.
        //

        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreFindFilesBySidCleanup;
    }

    Status = NcStreamHandleContextAllocAndAttach( FltObjects->Filter,
                                                  FltObjects->Instance,
                                                  FltObjects->FileObject,
                                                  &HandleContext );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreFindFilesBySidCleanup;
    }

    FLT_ASSERT( HandleContext != NULL );

    NcLockStreamHandleContext( HandleContext );
    UnlockContext = TRUE;

    FindBySidCtx = &HandleContext->FindBySidContext;

    //
    //  Typically we query on the user's file object.  We change file
    //  objects if the query is an ancestor of the mapping and we need to
    //  query the mapping itself.  We will need to translate names when
    //  this happens.  Since we cannot hold locks calling into the
    //  filesystem and don't want to drop locks during the calculation
    //  below, build a name for the mapping (pessimistically!) now.
    //
    //  We only really need this if BufferToFree != NULL, but this may
    //  change as soon as we drop the lock.
    //

    if (FindBySidCtx->RealFileObject == NULL) {

        OpenedName = UserRequestName;

    } else {

        PFILE_OBJECT RealFileObject = FindBySidCtx->RealFileObject;

        //
        //  Drop the lock.  Note that there are two possibilities for the
        //  FileObject - it either refers to the mapping, or it is NULL to
        //  indicate that we do not need to process the mapping, are not
        //  yet processing the mapping, or we are done with the operation.
        //
        //  In dropping the lock, it is possible that the FileObject to go
        //  to NULL.  However, since we know it was non-NULL, the only
        //  possibility if this occurs is that we are done with the
        //  enumeration.
        //

        ObReferenceObject( RealFileObject );
        NcUnlockStreamHandleContext( HandleContext );
        UnlockContext = FALSE;

        Status = NcGetFileNameInformation( NULL,
                                           RealFileObject,
                                           FltObjects->Instance,
                                           FLT_FILE_NAME_OPENED |
                                               FLT_FILE_NAME_QUERY_DEFAULT |
                                               FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                           &FileInfoInternalHandle );

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( RealFileObject );
            goto NcPreFindFilesBySidCleanup;
        }

        Status = FltParseFileNameInformation( FileInfoInternalHandle );

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( RealFileObject );
            goto NcPreFindFilesBySidCleanup;
        }

        OpenedName = FileInfoInternalHandle->Name;
        FLT_ASSERT( OpenedName.Length > 0 );
        if (OpenedName.Buffer[OpenedName.Length/sizeof(WCHAR) - 1] == NC_SEPARATOR) {
            OpenedName.Length -= sizeof(WCHAR);
        }

        //
        //  This may trigger a close, and must be done before we acquire the
        //  lock.
        //

        ObDereferenceObject( RealFileObject );

        //
        //  Now reacquire the lock so we can detect if we have a leftover
        //  buffer to munge and munge it safely.  Check whether the
        //  enumeration is still ongoing or if we are already complete.
        //

        NcLockStreamHandleContext( HandleContext );
        UnlockContext = TRUE;

        if (FindBySidCtx->RealFileObject == NULL) {

            FLT_ASSERT( FindBySidCtx->BufferToFree == NULL &&
                        FindBySidCtx->OutstandingRequests == 0 );

            ReturnValue = FLT_PREOP_COMPLETE;
            Status = STATUS_SUCCESS;
            SizeWeReturn = 0;

            goto NcPreFindFilesBySidCleanup;
        }
    }

    if (FindBySidCtx->BufferToFree != NULL) {

        //
        //  The user's buffer that we will be writing to
        //

        PFILE_NAME_INFORMATION DestBuffer = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBuffer;
        ULONG BufferSize = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength;

        ULONG InputConsumed;

        FLT_ASSERT( FindBySidCtx->BufferSize > 0 &&
                    FindBySidCtx->CurrentEntry < FindBySidCtx->BufferSize );

        //
        //  Whether we succeed or fail, we don't intend to let this request
        //  go to the filesystem if we have data that has not been returned
        //  yet.
        //

        ReturnValue = FLT_PREOP_COMPLETE;

        //
        //  If a driver above us has posted the request, we may need to
        //  map a system buffer for the call from its Mdl.  If not, we can
        //  use the user address passed in to the call.  Be sure to check
        //  the "user" address really does point to user memory.
        //

        if (Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress != NULL) {
            Status = FltLockUserBuffer( Data );

            if (!NT_SUCCESS( Status )) {

                goto NcPreFindFilesBySidCleanup;
            }

            DestBuffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress,
                                                       NormalPagePriority | MdlMappingNoExecute );

            if (DestBuffer == NULL) {

                Status = STATUS_NO_MEMORY;
                goto NcPreFindFilesBySidCleanup;
            }
        } else {

            try {

                if (Data->RequestorMode != KernelMode) {
                    ProbeForWrite( DestBuffer, BufferSize, sizeof( UCHAR ));
                }

            } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

                Status = STATUS_INVALID_USER_BUFFER;
            }

            if (!NT_SUCCESS( Status )) {

                goto NcPreFindFilesBySidCleanup;
            }
        }

        //
        //  Translate the strings in the buffer by applying the parent's path
        //  (either the opened handle's path or the mapping's path), then
        //  change the mapping as necessary.
        //

        Status = NcFindFilesBySidTranslateBuffers( InstanceContext,
                                                   IgnoreCase,
                                                   &UserRequestName,
                                                   &OpenedName,
                                                   Add2Ptr( FindBySidCtx->BufferToFree, FindBySidCtx->CurrentEntry ),
                                                   DestBuffer,
                                                   FindBySidCtx->BufferSize - FindBySidCtx->CurrentEntry,
                                                   BufferSize,
                                                   &InputConsumed,
                                                   &SizeWeReturn,
                                                   (BOOLEAN)UserOverlap.Ancestor );

        if (!NT_SUCCESS( Status )) {

            goto NcPreFindFilesBySidCleanup;
        }

        FindBySidCtx->CurrentEntry += InputConsumed;
        FLT_ASSERT( FindBySidCtx->CurrentEntry <= FindBySidCtx->BufferSize );

        if (FindBySidCtx->CurrentEntry >= FindBySidCtx->BufferSize) {

            //
            //  We've finished processing the leftover buffer.  Tear it down
            //  now.  Note that we will not attempt to call into the
            //  filesystem and fully pack the buffer; so long as we return
            //  something, that's good enough.
            //

            ExFreePoolWithTag( FindBySidCtx->BufferToFree, NC_TAG );
            FindBySidCtx->BufferToFree = NULL;
            FindBySidCtx->CurrentEntry = 0;
            FindBySidCtx->BufferSize = 0;
        }

        if (InputConsumed == 0) {

            FLT_ASSERT( SizeWeReturn == 0 );
            Status = STATUS_BUFFER_TOO_SMALL;
        }

    } else if (FindBySidCtx->RealFileObject) {

        //
        //  We don't have a buffer, but we do have a file object to redirect
        //  to.  This implies that we've finished reporting on the user's
        //  handle, and are now reporting on a redirected handle.  For
        //  simplicity, switch the file objects used by this request and
        //  send it down.
        //

        Data->Iopb->TargetFileObject = FindBySidCtx->RealFileObject;
        FltSetCallbackDataDirty( Data );

        //
        //  Pass a note back to ourselves about which file we're querying
        //  on.
        //

        *CompletionContext = FindBySidCtx->RealFileObject;

        //
        //  If we're sending down this request to the filesystem, bump the
        //  outstanding request count.
        //

        FLT_ASSERT( UnlockContext );
        FindBySidCtx->OutstandingRequests++;

        FLT_ASSERT( ReturnValue == FLT_PREOP_SYNCHRONIZE );

    } else {

        //
        //  Pass a note back to ourselves about which file we're querying
        //  on.
        //

        *CompletionContext = Data->Iopb->TargetFileObject;

        //
        //  If we're sending down this request to the filesystem, bump the
        //  outstanding request count.
        //

        FLT_ASSERT( UnlockContext );
        FindBySidCtx->OutstandingRequests++;

        FLT_ASSERT( ReturnValue == FLT_PREOP_SYNCHRONIZE );

    }

NcPreFindFilesBySidCleanup:

    if (ReturnValue == FLT_PREOP_COMPLETE) {

        Data->IoStatus.Status = Status;

        if (NT_SUCCESS( Status )) {

            Data->IoStatus.Information = SizeWeReturn;
        } else {

            Data->IoStatus.Information = 0;
        }
    }

    if (UnlockContext) {

        FLT_ASSERT( HandleContext != NULL );
        NcUnlockStreamHandleContext( HandleContext );

    }

    if (FileInfo != NULL) {

        FltReleaseFileNameInformation( FileInfo );
    }

    if (FileInfoInternalHandle != NULL) {

        FltReleaseFileNameInformation( FileInfoInternalHandle );
    }

    if (HandleContext != NULL) {

        FltReleaseContext( HandleContext );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    return ReturnValue;
}



FLT_POSTOP_CALLBACK_STATUS
NcPostFindFilesBySid (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    Routine is invoked when the user wants to query files owned by
    a particular SID under a subtree.  For this call, we must return
    paths relative to the user's handle.

    This routine must consider the following three cases:

    1. If the handle is an ancestor or match of the real mapping, we may
       need to remove entries.

    2. If the handle is an ancestor or match of the user mapping, we may
       need to insert entries.  We implement this in two phases: first,
       we enumerate all results under the user's handle; when complete,
       we open a handle to the mapping and enumerate all results under
       that.

    3. If the handle is an ancestor of both, we may need to transform
       entries.

    Logically, we take the data returned from the filesystem (if any),
    transform the buffers, and return it to the user.  If we have more than
    we can return, we add it to our leftover buffer.  If we don't have any
    data from the filesystem, we check if we have any data left over in the
    leftover buffer, transform and return that.  If we don't have any data
    in the leftover buffer, we check if we need to enumerate from the
    mapping handle (in number 2 above), and if so, open and query that,
    transform the results, and return.  If we've already done that and
    still have no data, we close our handles, and return an empty buffer
    to the caller, indicating the series of requests is complete.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure
        containing opaque handles to this filter, instance, its
        associated volume and file object.

    CompletionContext - The context for the completion routine for
        this operation.  We set this to the file object we are enumerating
        under for find files by SID.  NcPreFindFilesBySid ensures that this
		will never be NULL if the post-op is invoked.

    Flags - The flags for this operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NTSTATUS Status;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );
    PNC_STREAM_HANDLE_CONTEXT HandleContext = NULL;
    PNC_FIND_BY_SID_CONTEXT FindBySidCtx = NULL;
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    PFLT_FILE_NAME_INFORMATION FileInfoInternalHandle = NULL;

    //
    //  The FileObject that this request was sent to.
    //

    PFILE_OBJECT FileObject = (PFILE_OBJECT)CompletionContext;

    //
    //  The user's buffer that we will be writing to, and the system copy
    //  of the data returned by the file system which we will read from.
    //

    PFILE_NAME_INFORMATION DestBuffer = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBuffer;
    PFILE_NAME_INFORMATION SourceBuffer = NULL;
    ULONG SourceBufferSize;

    //
    //  The opened path's relation to the mappings.
    //

    NC_PATH_OVERLAP RealOverlap;
    NC_PATH_OVERLAP UserOverlap;

    //
    //  Size returned from the filesystem to us, size of the user's buffer
    //  that we can legitimately write to, the amount we actually wrote into
    //  the user's buffer, and the amount of the filesystem's buffer we have
    //  processed so far.
    //

    ULONG SizeActuallyReturned = (ULONG)Data->IoStatus.Information;
    ULONG BufferSize = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength;
    ULONG InputConsumed;
    ULONG SizeWeReturn;

    //
    //  The name of the object opened by the user, and the path of the object
    //  we're issuing queries on.  These are typically the same, but will
    //  differ when we're injecting entries (user opens an ancestor of the
    //  user mapping, but we're sending queries to the real mapping.)
    //

    UNICODE_STRING UserRequestName;
    UNICODE_STRING OpenedName;

    BOOLEAN InjectionRequired = FALSE;
    BOOLEAN UnlockContext = FALSE;

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  If the operation has already failed, we're not interested in attempting
    //  to process it.
    //

    if (!NT_SUCCESS( Data->IoStatus.Status )) {

        Status = Data->IoStatus.Status;
        goto NcPostFindFilesBySidCleanup;
    }

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        goto NcPostFindFilesBySidCleanup;
    }

    FLT_ASSERT( BufferSize >= SizeActuallyReturned );
    if (BufferSize < sizeof( FILE_NAME_INFORMATION )) {

        //
        //  This somewhat strange code is used to maintain parity with the
        //  filesystem.  Note that we checked this in pre, so this check
        //  should be redundant.
        //

        FLT_ASSERT( BufferSize >= sizeof( FILE_NAME_INFORMATION ));
        Status = STATUS_INVALID_USER_BUFFER;
        goto NcPostFindFilesBySidCleanup;
    }

    //
    //  Get the file handle's name.  Names returned via this FSCTL are
    //  relative to the handle used to obtain them.  We will need to
    //  construct names by combining these values.
    //

    Status = NcGetFileNameInformation( Data,
                                       NULL,
                                       NULL,
                                       FLT_FILE_NAME_OPENED |
                                           FLT_FILE_NAME_QUERY_DEFAULT |
                                           FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                       &FileInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcPostFindFilesBySidCleanup;
    }

    Status = FltParseFileNameInformation( FileInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcPostFindFilesBySidCleanup;
    }

    //
    //  As an optimization, check if the handle queried on is an
    //  ancestor of either mapping.  Cases to consider:
    //
    //  1. If the handle is an ancestor or match of the real
    //     mapping, we may need to remove entries.
    //
    //  2. If the handle is an ancestor or match of the user
    //     mapping, we may need to insert entries.
    //
    //  3. If the handle is an ancestor of both, we may need
    //     to transform entries.
    //
    //  4. If the handle is not an ancestor of either, we have
    //     no work to do.  As an optimization, we can stop now
    //     if this is the case.
    //

    NcComparePath( &FileInfo->Name,
                   &InstanceContext->Mapping.UserMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &UserOverlap );

    NcComparePath( &FileInfo->Name,
                   &InstanceContext->Mapping.RealMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &RealOverlap );

    if (!(UserOverlap.Ancestor || RealOverlap.Ancestor)) {

        //
        //  If we are not an ancestor, our post callback should
        //  not have been invoked.
        //

        FLT_ASSERT( UserOverlap.Ancestor || RealOverlap.Ancestor );

        //
        //  In case 4 above, leave now.
        //

        Status = STATUS_SUCCESS;
        goto NcPostFindFilesBySidCleanup;
    }

    //
    //  If the path that was used for the query ends in a trailing slash
    //  (ie., the root directory), shave it off here.  This keeps our logic
    //  simple, rather than dealing with this over and over.
    //

    UserRequestName = FileInfo->Name;
    FLT_ASSERT( UserRequestName.Length > 0 );
    if (UserRequestName.Buffer[UserRequestName.Length/sizeof(WCHAR) - 1] == NC_SEPARATOR) {
        UserRequestName.Length -= sizeof(WCHAR);
    }

    if (UserOverlap.Ancestor && !RealOverlap.Ancestor) {

        InjectionRequired = TRUE;
    }

    Status = NcStreamHandleContextAllocAndAttach( FltObjects->Filter,
                                                  FltObjects->Instance,
                                                  FltObjects->FileObject,
                                                  &HandleContext );

    if (!NT_SUCCESS( Status )) {

        goto NcPostFindFilesBySidCleanup;
    }

    FLT_ASSERT( HandleContext != NULL );

    NcLockStreamHandleContext( HandleContext );
    UnlockContext = TRUE;

    FindBySidCtx = &HandleContext->FindBySidContext;

    //
    //  If we're being notified of a filesystem completing a request, we'd
    //  better have seen it.
    //

    FLT_ASSERT( FindBySidCtx->OutstandingRequests > 0 );

    //
    //  If another driver has posted the request, we may need to map a
    //  system buffer for the call from its Mdl.  If not, we can use the
    //  user address passed in to the call.  Be sure to check the "user"
    //  address really does point to user memory.
    //

    if (Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress != NULL) {
        Status = FltLockUserBuffer( Data );

        if (!NT_SUCCESS( Status )) {

            goto NcPostFindFilesBySidCleanup;
        }

        DestBuffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress,
                                                   NormalPagePriority | MdlMappingNoExecute );

        if (DestBuffer == NULL) {

            Status = STATUS_NO_MEMORY;
            goto NcPostFindFilesBySidCleanup;
        }

    } else {

        try {

            if (Data->RequestorMode != KernelMode) {
                ProbeForWrite( DestBuffer, BufferSize, sizeof( UCHAR ));
            }

        } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

            Status = STATUS_INVALID_USER_BUFFER;
        }

        if (!NT_SUCCESS( Status )) {

            goto NcPostFindFilesBySidCleanup;
        }
    }

NcPostFindFilesBySidMungeBuffer:

    //
    //  In the typical case, we're querying on the user's object.  In that
    //  case, the name we're querying on is the same name that will be the
    //  basis for returned results.
    //

    if (FileObject != FindBySidCtx->RealFileObject) {

        OpenedName = UserRequestName;

    } else {

        if (UnlockContext) {
            NcUnlockStreamHandleContext( HandleContext );
            UnlockContext = FALSE;
        }

        //
        //  We should query this only once, either because we just opened this
        //  object and are looping, or because it was previously set up and
        //  we do not loop.  If we are looping, we did not reacquire the lock,
        //  and did not need to drop it above.
        //

        FLT_ASSERT( FileInfoInternalHandle == NULL );

        Status = NcGetFileNameInformation( NULL,
                                           FindBySidCtx->RealFileObject,
                                           FltObjects->Instance,
                                           FLT_FILE_NAME_OPENED |
                                               FLT_FILE_NAME_QUERY_DEFAULT |
                                               FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                           &FileInfoInternalHandle );

        if (!NT_SUCCESS( Status )) {

            goto NcPostFindFilesBySidCleanup;
        }

        Status = FltParseFileNameInformation( FileInfoInternalHandle );

        if (!NT_SUCCESS( Status )) {

            goto NcPostFindFilesBySidCleanup;
        }

        OpenedName = FileInfoInternalHandle->Name;
        FLT_ASSERT( OpenedName.Length > 0 );
        if (OpenedName.Buffer[OpenedName.Length/sizeof(WCHAR) - 1] == NC_SEPARATOR) {
            OpenedName.Length -= sizeof(WCHAR);
        }

    }

    if (!UnlockContext) {
        NcLockStreamHandleContext( HandleContext );
        UnlockContext = TRUE;
    }

    //
    //  If the filesystem returned something, go ahead and try to munge it.
    //

    if (SizeActuallyReturned > sizeof( FILE_NAME_INFORMATION ) - sizeof(WCHAR)) {

        //
        //  Allocate a new buffer and copy the contents.  Note that this is
        //  particularly important with this call, since it's not system
        //  buffered; the contents are free to change underneath us.  This
        //  allocation protects us against that, but we still must be
        //  paranoid touching the buffer, since we cannot trust that it has
        //  any integrity at this point.
        //
        //  Make sure we allocate enough space to deal with this request,
        //  plus any leftover buffer.  If we don't process this entire buffer
        //  right now, we'll need to store our remnants with accumulated
        //  remnants.
        //
        //  The #pragma is a notation to the static code analyzer to not worry
        //  that we're apparently leaking SourceBuffer.  It gets put in to
        //  FindBySidCtx, which is a part of our stream handle context.  When
        //  the stream handle context is eventually torn down this allocated
        //  memory will be freed.  Naturally you must take care that on any
        //  paths where SourceBuffer doesn't make it in to the stream handle
        //  context you deallocate SourceBuffer before this routine exits.
        //

        Status = RtlULongAdd( SizeActuallyReturned,
                              FindBySidCtx->BufferSize,
                              &SourceBufferSize );

        if (NT_SUCCESS( Status )) {

            Status = RtlULongSub( SourceBufferSize,
                                  FindBySidCtx->CurrentEntry,
                                  &SourceBufferSize );
        }

        if (!NT_SUCCESS( Status )) {

            goto NcPostFindFilesBySidCleanup;
        }

#pragma warning(suppress: __WARNING_MEMORY_LEAK)
        SourceBuffer = ExAllocatePoolZero( PagedPool,
                                           SourceBufferSize,
                                           NC_TAG );

        if (SourceBuffer == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto NcPostFindFilesBySidCleanup;
        }

        try {

            RtlCopyMemory( SourceBuffer, DestBuffer, SizeActuallyReturned );

        } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

            Status = STATUS_INVALID_USER_BUFFER;
            goto NcPostFindFilesBySidCleanup;
        }

        Status = NcFindFilesBySidTranslateBuffers( InstanceContext,
                                                   IgnoreCase,
                                                   &UserRequestName,
                                                   &OpenedName,
                                                   SourceBuffer,
                                                   DestBuffer,
                                                   SizeActuallyReturned,
                                                   BufferSize,
                                                   &InputConsumed,
                                                   &SizeWeReturn,
                                                   (BOOLEAN)UserOverlap.Ancestor );

        if (!NT_SUCCESS( Status )) {

            goto NcPostFindFilesBySidCleanup;
        }

        if (InputConsumed == 0) {
            FLT_ASSERT( SizeWeReturn == 0 );
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        if (InputConsumed < SizeActuallyReturned) {

            ULONG RemnantsSize = SizeActuallyReturned;

            //
            //  We have successfully processed a portion of the buffer,
            //  but more remains.  Append any previously leftover portion
            //  of buffer to our own, and attach the combined remnants to
            //  our handle context for later processing.  Importantly,
            //  don't free the buffer here.
            //

            if (FindBySidCtx->BufferToFree != NULL) {

                RtlCopyMemory( Add2Ptr( SourceBuffer, SizeActuallyReturned ),
                               Add2Ptr( FindBySidCtx->BufferToFree, FindBySidCtx->CurrentEntry ),
                               FindBySidCtx->BufferSize - FindBySidCtx->CurrentEntry );

                RemnantsSize += FindBySidCtx->BufferSize - FindBySidCtx->CurrentEntry;

                ExFreePoolWithTag( FindBySidCtx->BufferToFree, NC_TAG );

            }

            FindBySidCtx->BufferToFree = SourceBuffer;
            FindBySidCtx->BufferSize = RemnantsSize;
            FindBySidCtx->CurrentEntry = InputConsumed;

            SourceBuffer = NULL;
        }

    } else {

        //
        //  We have nothing from the filesystem.  Now see what else we need
        //  to do:
        //
        //  1. If we have anything left in the buffer, return it now.  This
        //     should be rare, but can happen with multiple async requests
        //     in different threads on the same handle.  If we saw nothing
        //     in the buffer when this request started (in pre), but there
        //     is data now, another simultaneous request must have generated
        //     it.
        //
        //  2. If this request was sent to the mapping, we are done.  Tear
        //     down mapping state if this is the final outstanding request.
        //
        //  3. If we need to return results from the mapping, we need to
        //     set up support for that now.  It is important that we return
        //     all buffered results first, so that name translation is still
        //     accurate.
        //

        FLT_ASSERT( SizeActuallyReturned == 0 );
        SizeWeReturn = 0;

        if (FindBySidCtx->BufferToFree != NULL) {

            //
            //  We need to clean up any outstanding buffer now.  Normally
            //  this will happen in a pre operation, but we must return some
            //  data in order for the caller to call us again, and we can't
            //  set up or tear down the mapping or any existing data in the
            //  buffer will be returned with wrong names, since the root of
            //  the operation has changed.
            //

            FLT_ASSERT( FindBySidCtx->BufferSize > 0 &&
                        FindBySidCtx->CurrentEntry < FindBySidCtx->BufferSize );

            //
            //  Translate the strings in the buffer by applying the parent's
            //  path, then change the mapping as necessary.
            //

            Status = NcFindFilesBySidTranslateBuffers( InstanceContext,
                                                       IgnoreCase,
                                                       &UserRequestName,
                                                       &OpenedName,
                                                       Add2Ptr( FindBySidCtx->BufferToFree, FindBySidCtx->CurrentEntry ),
                                                       DestBuffer,
                                                       FindBySidCtx->BufferSize - FindBySidCtx->CurrentEntry,
                                                       BufferSize,
                                                       &InputConsumed,
                                                       &SizeWeReturn,
                                                       (BOOLEAN)UserOverlap.Ancestor );

            if (!NT_SUCCESS( Status )) {

                goto NcPostFindFilesBySidCleanup;
            }

            FindBySidCtx->CurrentEntry += InputConsumed;
            FLT_ASSERT( FindBySidCtx->CurrentEntry <= FindBySidCtx->BufferSize );

            if (FindBySidCtx->CurrentEntry >= FindBySidCtx->BufferSize) {

                //
                //  We've finished processing the leftover buffer.  Tear it
                //  down now.  Note that we will not attempt to call into
                //  the filesystem and fully pack the buffer; so long as we
                //  return something, that's good enough.
                //

                ExFreePoolWithTag( FindBySidCtx->BufferToFree, NC_TAG );
                FindBySidCtx->BufferToFree = NULL;
                FindBySidCtx->CurrentEntry = 0;
                FindBySidCtx->BufferSize = 0;

            }

            if (InputConsumed == 0) {

                FLT_ASSERT( SizeWeReturn == 0 );
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            //
            //  As an optimization, we could set up the handle here.  Doing
            //  so would save one trip to the filesystem, since the next
            //  request on this handle will go directly to the mapping.  For
            //  simplicity, we skip this, so the next request will go to the
            //  filesystem (which will tell us it has nothing to say), and
            //  we'll perform the create, then retry the request.
            //

            goto NcPostFindFilesBySidCleanup;
        }

        if (FileObject == FindBySidCtx->RealFileObject) {

            //
            //  We've completed the request on the user's handle, and we've
            //  completed the request on the real handle.  We are done.
            //
            //  If this is the final outstanding request from the completed
            //  operation, start cleaning everything up.
            //

            if (FindBySidCtx->OutstandingRequests == 1) {

                HANDLE HandleToClose = FindBySidCtx->RealHandle;
                PFILE_OBJECT FileObjectToDereference = FindBySidCtx->RealFileObject;

                FindBySidCtx->RealHandle = NULL;
                FindBySidCtx->RealFileObject = NULL;

                NcUnlockStreamHandleContext( HandleContext );
                UnlockContext = FALSE;

                FltClose( HandleToClose );
                ObDereferenceObject( FileObjectToDereference );
            }

        } else if (InjectionRequired) {

            OBJECT_ATTRIBUTES MappingAttributes;
            HANDLE MappingHandle = NULL;
            PFILE_OBJECT MappingFileObject = NULL;
            IO_STATUS_BLOCK MappingStatusBlock;

            FLT_ASSERT( FindBySidCtx->RealFileObject == NULL );

            //
            //  We have finished enumerating from the filesystem, and we
            //  need to enumerate from the mapping.  At this point, the
            //  mapping has not been set up yet, so we do that now.
            //
            //  Because we can't hold a lock doing this, we do so
            //  speculatively: two threads may end up doing this work, and
            //  one may be thrown away.
            //
            //  Note that if two threads are racing, we should have an
            //  OutstandingRequests count of 2, so the result cannot be torn
            //  down by the other thread.
            //

            FLT_ASSERT( FindBySidCtx->RealHandle == NULL );
            FLT_ASSERT( FindBySidCtx->OutstandingRequests > 0 );

            NcUnlockStreamHandleContext( HandleContext );
            UnlockContext = FALSE;

            //
            //  Open the mapping.  We're done returning data on the user's
            //  handle, but we still need to return data from the mapping.
            //

            InitializeObjectAttributes( &MappingAttributes,
                                        &InstanceContext->Mapping.RealMapping.LongNamePath.FullPath,
                                        OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                        NULL,
                                        NULL);

            Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                         FltObjects->Instance,                 // Instance
                                         &MappingHandle,                       // Returned Handle
                                         &MappingFileObject,                   // Returned FileObject
                                         FILE_READ_ATTRIBUTES|FILE_TRAVERSE,   // Desired Access
                                         &MappingAttributes,                   // object attributes
                                         &MappingStatusBlock,                  // Returned IOStatusBlock
                                         0,                                    // Allocation Size
                                         FILE_ATTRIBUTE_NORMAL,                // File Attributes
                                         0,                                    // Share Access
                                         FILE_OPEN,                            // Create Disposition
                                         FILE_DIRECTORY_FILE,                  // Create Options
                                         NULL,                                 // Ea Buffer
                                         0,                                    // EA Length
                                         IO_IGNORE_SHARE_ACCESS_CHECK,         // Flags
                                         FltObjects->FileObject );             // Transaction info.

            if (!NT_SUCCESS( Status )) {

                if ( Status == STATUS_OBJECT_PATH_NOT_FOUND ||
                     Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

                    Status = STATUS_SUCCESS;
                }
                goto NcPostFindFilesBySidCleanup;
            }

            NcLockStreamHandleContext( HandleContext );
            UnlockContext = TRUE;

            if (FindBySidCtx->RealHandle == NULL) {

                //
                //  Store our object into the context.
                //

                FindBySidCtx->RealHandle = MappingHandle;
                FindBySidCtx->RealFileObject = MappingFileObject;

                NcUnlockStreamHandleContext( HandleContext );
                UnlockContext = FALSE;

            } else {

                //
                //  Another thread beat us to the punch.  Tear down
                //  our state and reload from theirs.
                //

                NcUnlockStreamHandleContext( HandleContext );
                UnlockContext = FALSE;

                FltClose( MappingHandle );
                ObDereferenceObject( MappingFileObject );
            }

            MappingHandle = NULL;
            MappingFileObject = NULL;

            FileObject = FindBySidCtx->RealFileObject;

            Status = FltFsControlFile( FltObjects->Instance,
                                       FindBySidCtx->RealFileObject,
                                       FSCTL_FIND_FILES_BY_SID,
                                       Data->Iopb->Parameters.FileSystemControl.Neither.InputBuffer,
                                       Data->Iopb->Parameters.FileSystemControl.Neither.InputBufferLength,
                                       Data->Iopb->Parameters.FileSystemControl.Neither.OutputBuffer,
                                       Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength,
                                       &SizeActuallyReturned );

            if (!NT_SUCCESS( Status )) {

                goto NcPostFindFilesBySidCleanup;
            }

            goto NcPostFindFilesBySidMungeBuffer;

        }
    }

    Data->IoStatus.Information = SizeWeReturn;

NcPostFindFilesBySidCleanup:

    if (SourceBuffer != NULL) {

        ExFreePoolWithTag( SourceBuffer, NC_TAG );
    }

    //
    //  If this function is being called, we were counted as an outstanding
    //  request.  If we made it far enough to have our handle context, we
    //  can un-count ourselves now.  If we failed before that, we remain
    //  counted, and cleanup occurs when the handle is closed.
    //

    FLT_ASSERT( FindBySidCtx != NULL );
    if (FindBySidCtx != NULL) {
        FLT_ASSERT( HandleContext != NULL );

        if (!UnlockContext) {
            NcLockStreamHandleContext( HandleContext );
            UnlockContext = TRUE;
        }

        FLT_ASSERT( FindBySidCtx->OutstandingRequests > 0 );
        FindBySidCtx->OutstandingRequests--;
    }

    if (UnlockContext) {

        FLT_ASSERT( HandleContext != NULL );
        NcUnlockStreamHandleContext( HandleContext );
    }

    if (FileInfo != NULL) {

        FltReleaseFileNameInformation( FileInfo );
    }

    if (FileInfoInternalHandle != NULL) {

        FltReleaseFileNameInformation( FileInfoInternalHandle );
    }

    if (HandleContext != NULL) {

        FltReleaseContext( HandleContext );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (!NT_SUCCESS( Status )) {
        Data->IoStatus.Status = Status;
        Data->IoStatus.Information = 0;
    }

    UNREFERENCED_PARAMETER( Flags );

    return FLT_POSTOP_FINISHED_PROCESSING;

}

#if FLT_MGR_WIN7
FLT_POSTOP_CALLBACK_STATUS
NcPostLookupStreamFromCluster (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    Routine is invoked when the user wants to find a stream owning
    a particular cluster.  For this call, paths are absolute relative to
    the start of the volume.

    Note that this call is system buffered.  This means we don't need
    to process MDLs, or perform probes, or treat our input data as
    suspect.

    All we need to do is walk through the returned data, change names that
    refer to the mappings, and return the result.  Since the lengths may
    change, we may return fewer entries, but will still tell the caller
    the number of entries that actually exist and the size of the buffer
    to obtain all of them.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure
        containing opaque handles to this filter, instance, its
        associated volume and file object.

    CompletionContext - The context for the completion routine for
        this operation.

    Flags - The flags for this operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    PLOOKUP_STREAM_FROM_CLUSTER_OUTPUT DestBuffer = Data->Iopb->Parameters.FileSystemControl.Buffered.SystemBuffer;
    PLOOKUP_STREAM_FROM_CLUSTER_ENTRY  SourceEntry = NULL;
    PLOOKUP_STREAM_FROM_CLUSTER_ENTRY  DestEntry = NULL;
    PLOOKUP_STREAM_FROM_CLUSTER_ENTRY  PrevDestEntry = NULL;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NTSTATUS Status;
    NC_PATH_OVERLAP Overlap;
    ULONG SizeActuallyReturned = (ULONG)Data->IoStatus.Information;
    ULONG BufferSize = Data->Iopb->Parameters.FileSystemControl.Buffered.OutputBufferLength;
    ULONG EntryLength;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );
    PLOOKUP_STREAM_FROM_CLUSTER_OUTPUT SourceBuffer = NULL;
    UNICODE_STRING NameString;
    UNICODE_STRING Remainder;

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  If the operation has already failed, we're not interested in attempting
    //  to process it.
    //

    if (!NT_SUCCESS( Data->IoStatus.Status ) || SizeActuallyReturned == 0) {

        Status = Data->IoStatus.Status;
        goto NcPostLookupStreamFromClusterCleanup;
    }

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        goto NcPostLookupStreamFromClusterCleanup;
    }

    if (BufferSize < sizeof( LOOKUP_STREAM_FROM_CLUSTER_OUTPUT )) {

        Status = STATUS_INVALID_PARAMETER;
        goto NcPostLookupStreamFromClusterCleanup;
    }

    //
    //  We take a copy of the buffer so that we can re-write the request's
    //  buffer with our modified data.
    //

    SourceBuffer = ExAllocatePoolZero( PagedPool,
                                       SizeActuallyReturned,
                                       NC_TAG );

    if (SourceBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcPostLookupStreamFromClusterCleanup;
    }

    RtlCopyMemory( SourceBuffer, DestBuffer, SizeActuallyReturned );

    //
    //  We initialize this for the first entry.  If we don't actually
    //  copy an entry, it is re-zeroed later.
    //

    DestBuffer->Offset = AlignToSize( sizeof( LOOKUP_STREAM_FROM_CLUSTER_OUTPUT ), 8);
    DestBuffer->NumberOfMatches = 0;
    DestBuffer->BufferSizeRequired = AlignToSize( sizeof( LOOKUP_STREAM_FROM_CLUSTER_OUTPUT ), 8);

    //
    //  Since this is a buffered FSCTL, we treat the output buffer
    //  as well formed data.  This enables us to make a few assumptions.
    //  Firstly, we don't expect to walk off the end of the allocation.
    //  It follows that at least for now, both source and destination
    //  have the same offsets.
    //

    if (SourceBuffer->Offset > 0) {
        SourceEntry = Add2Ptr( SourceBuffer, SourceBuffer->Offset );
        DestEntry = Add2Ptr( DestBuffer, SourceBuffer->Offset );
    }

    while (SourceEntry) {

        RtlInitUnicodeString( &NameString, SourceEntry->FileName );

        //
        //  Check if the path refers to a location within the real side of
        //  the mapping.  If it does, translate the data to the user side
        //  of the mapping.
        //

        NcComparePath( &NameString,
                       &InstanceContext->Mapping.RealMapping,
                       &Remainder,
                       IgnoreCase,
                       FALSE,
                       &Overlap );

        if (Overlap.InMapping) {

            Status = NcConstructPath( &InstanceContext->Mapping.UserMapping,
                                      &Remainder,
                                      FALSE,
                                      &NameString );

            if (!NT_SUCCESS( Status )) {
                goto NcPostLookupStreamFromClusterCleanup;
            }
        }

        //
        //  Note that we're not allowing for NULL, because the structure
        //  already includes one WCHAR.
        //

        EntryLength = AlignToSize( sizeof( LOOKUP_STREAM_FROM_CLUSTER_ENTRY ) +
                                   NameString.Length, 8 );

        //
        //  If it fits, copy the data into the returned entry, and advance
        //  to the next entry to return.
        //

        if (DestBuffer->BufferSizeRequired + EntryLength <= BufferSize) {

            DestEntry->Flags = SourceEntry->Flags;
            DestEntry->Reserved = SourceEntry->Reserved;
            DestEntry->Cluster = SourceEntry->Cluster;
            RtlCopyMemory( DestEntry->FileName,
                           NameString.Buffer,
                           NameString.Length );

            DestEntry->FileName[ NameString.Length / sizeof(WCHAR)] = L'\0';

            DestEntry->OffsetToNext = EntryLength;

            PrevDestEntry = DestEntry;
            DestEntry = Add2Ptr( DestEntry, DestEntry->OffsetToNext );
        }

        //
        //  If we needed to munge the name, we allocated pool to store the
        //  result.  Free that pool now.
        //

        if (Overlap.InMapping) {
            ExFreePoolWithTag( NameString.Buffer, NC_GENERATE_NAME_TAG );
        }

        //
        //  We must keep track of the required buffer size and number of
        //  matches regardless of whether it is returned.
        //

        DestBuffer->BufferSizeRequired += EntryLength;
        DestBuffer->NumberOfMatches++;

        //
        //  Advance to the next source entry.  We assume our input is
        //  well formed, and therefore, we do not expect to walk off the
        //  end of this buffer.
        //

        if (SourceEntry->OffsetToNext > 0) {
            SourceEntry = Add2Ptr( SourceEntry, SourceEntry->OffsetToNext );
        } else {
            SourceEntry = NULL;
        }
    }

    if (PrevDestEntry) {
        PrevDestEntry->OffsetToNext = 0;
    } else {
        DestBuffer->Offset = 0;
    }

    Data->IoStatus.Information = min( DestBuffer->BufferSizeRequired, BufferSize );

    //
    //  Since this filter may change the length of names returned, we may
    //  also change the layout and bytes required.  We will not, however,
    //  change the number of matches; this value refers to the total number
    //  that exist, not the number that can fit in the buffer.
    //

    FLT_ASSERT( DestBuffer->NumberOfMatches == SourceBuffer->NumberOfMatches );

NcPostLookupStreamFromClusterCleanup:

    if (SourceBuffer != NULL) {

        ExFreePoolWithTag( SourceBuffer, NC_TAG );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (!NT_SUCCESS( Status )) {
        Data->IoStatus.Status = Status;
        Data->IoStatus.Information = 0;
    }

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FLT_POSTOP_FINISHED_PROCESSING;

}
#endif

NTSTATUS
NcUsnTranslateBuffers (
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ BOOLEAN IgnoreCase,
    _In_ ULONGLONG RealMappingParentId,
    _In_ ULONGLONG UserMappingParentId,
    _In_ PUSN_RECORD InputSystemBuffer,
    _Out_writes_bytes_to_(OutputBufferLength, *OutputBufferWritten) PUSN_RECORD OutputUserBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG InputBufferConsumed,
    _Out_ PULONG OutputBufferWritten
    )
/*++

Routine Description:

    This routine is used to transform buffers from a filesystem view to the
    user view.  We must replace both the parent ID and link name of any
    records referring to the real mapping with corresponding values for the
    user mapping.

Arguments:

    InstanceContext - Pointer to the context describing this instance of the
        filter.

    IgnoreCase - TRUE if comparisons should be case insensitive, FALSE if
        comparisons should be case sensitive.  TODO: Is this required?

    RealMappingParentId - The file ID corresponding to the real mapping
        parent.

    UserMappingParentId - The file ID corresponding to the user mapping
        parent.

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

Return Value:

    The return value is the Status of the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    //  Pointers into the above buffers used as we read entries from
    //  one buffer and write them to the other.
    //

    PUSN_RECORD SourceEntry = NULL;
    PUSN_RECORD DestEntry = NULL;

    //
    //  Points to a new name if we are required to substitute names for
    //  this record.  If no substitution is required, it is NULL.
    //

    PUNICODE_STRING NewLink;

    //
    //  The length of this record, in bytes.  We use this field to refer
    //  to source and destination records at different times.
    //

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
            //  Assume we're not transforming any records, which is the
            //  common case.
            //

            NewLink = NULL;

            //
            //  If we're about to walk off the end of the buffer, get out
            //  now.  This should only happen if the user has modified the
            //  buffer after it was returned from the filesystem and before
            //  we had a chance to copy it.
            //
            //  Firstly check we have enough space for a record, and having
            //  established that we have enough space to read the first few
            //  bytes containing the record length, check against that.  Use
            //  safe math routines for the untrusted SourceEntry values.
            //

            Status = RtlULongAdd( *InputBufferConsumed,
                                  sizeof(USN_RECORD),
                                  &UlongResult );

            if (!NT_SUCCESS( Status ) ||
                (UlongResult > InputBufferLength)) {

                goto FailedBufferCheck;
            }

            Status = RtlULongAdd( *InputBufferConsumed,
                                  SourceEntry->RecordLength,
                                  &UlongResult );

            if (!NT_SUCCESS( Status ) ||
                (UlongResult > InputBufferLength)) {

                goto FailedBufferCheck;
            }

            Status = RtlULongAdd( *InputBufferConsumed,
                                  SourceEntry->FileNameOffset,
                                  &UlongResult );

            if (NT_SUCCESS( Status )) {

                Status = RtlULongAdd( UlongResult,
                                      SourceEntry->FileNameLength,
                                      &UlongResult );
            }

            if (!NT_SUCCESS( Status ) ||
                (UlongResult > InputBufferLength) ||
                (SourceEntry->FileNameOffset < FIELD_OFFSET( USN_RECORD, FileName ))) {

FailedBufferCheck:
                FLT_ASSERT( FALSE );
                SourceEntry = NULL;
                Status = STATUS_INVALID_USER_BUFFER;
                break;
            }

            //
            //  Assume that the destination record will be the same size as
            //  the source record.  Note that SourceEntry->RecordLength is untrusted,
            //  so now EntryLength is.
            //

            EntryLength = SourceEntry->RecordLength;

            //
            //  We have encountered a USN record with an incompatible
            //  version.  In theory, MinorVersion changes are compatible,
            //  MajorVersion changes are not.  In practice, a MinorVersion
            //  change could include offsets to additional data which we have
            //  no knowledge of, so we can never be sure that we're not
            //  returning corrupt records on a MinorVersion change.  Rather
            //  than do that, we just give up.
            //

            if (SourceEntry->MajorVersion != 2 ||
                SourceEntry->MinorVersion != 0) {

                SourceEntry = NULL;
                Status = STATUS_NOT_IMPLEMENTED;
                break;
            }

            //
            //  Check if this is a record we need to transform.  If so, adjust
            //  the length of the destination record appropriately, and record
            //  which name we should transform to.
            //

            if (SourceEntry->ParentFileReferenceNumber == RealMappingParentId &&
                (SourceEntry->FileNameLength == InstanceContext->Mapping.RealMapping.LongNamePath.FinalComponentName.Length ||
                 SourceEntry->FileNameLength == InstanceContext->Mapping.RealMapping.ShortNamePath.FinalComponentName.Length)) {

                UNICODE_STRING TempString;

                TempString.Buffer = Add2Ptr( SourceEntry, SourceEntry->FileNameOffset );
                TempString.Length = TempString.MaximumLength = SourceEntry->FileNameLength;

                if (RtlCompareUnicodeString( &TempString,
                                             &InstanceContext->Mapping.RealMapping.LongNamePath.FinalComponentName,
                                             IgnoreCase ) == 0) {

                    NewLink = &InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName;

                } else
                if (RtlCompareUnicodeString( &TempString,
                                             &InstanceContext->Mapping.RealMapping.ShortNamePath.FinalComponentName,
                                             IgnoreCase ) == 0) {

                    NewLink = &InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName;

                }

                if (NewLink != NULL) {

                    EntryLength = AlignToSize( SourceEntry->FileNameOffset + NewLink->Length, 8 );
                }
            }

            //
            //  If the current entry would overflow the remaining output buffer
            //  then we've done all we can.  Return now to let our caller deal
            //  with the remaining buffer.
            //

            Status = RtlULongAdd( EntryLength,
                                  *OutputBufferWritten,
                                  &UlongResult );

            if (!NT_SUCCESS( Status ) ||
                (UlongResult > OutputBufferLength)) {

                break;
            }

            //
            //  If we're not transforming (the common case) copy the entire
            //  record blindly from source to destination.  If we are
            //  transforming, copy the record, switch the file ID, then
            //  copy the name.
            //

            if (NewLink == NULL) {

                RtlCopyMemory( DestEntry,
                               SourceEntry,
                               EntryLength );

            } else {


                //
                //  Copy all of the record up to the file name offset.  This
                //  is the most resilient approach to a minor version change
                //  which adds new fields.  We will preserve the minor
                //  version number, and preserve the fields in this process.
                //  Note that this is a moot exercise here, since we are
                //  failing on unknown minor versions above.
                //

                RtlCopyMemory( DestEntry,
                               SourceEntry,
                               SourceEntry->FileNameOffset );

                DestEntry->ParentFileReferenceNumber = UserMappingParentId;

                DestEntry->FileNameLength = NewLink->Length;
                DestEntry->FileNameOffset = FIELD_OFFSET( USN_RECORD, FileName );
                DestEntry->RecordLength = EntryLength;

                //
                //  Since the DestBuffer is user-exposed, we cannot trust
                //  its contents immediately after writing them.  Here, we
                //  calculate the file offset from the source entry because
                //  that is system buffered.
                //

                RtlCopyMemory( Add2Ptr( DestEntry, SourceEntry->FileNameOffset ),
                               NewLink->Buffer,
                               NewLink->Length );
            }

            Status = RtlULongAdd( *InputBufferConsumed,
                                  SourceEntry->RecordLength,
                                  InputBufferConsumed );

            PointerResult = Add2Ptr( SourceEntry, SourceEntry->RecordLength );

            //
            //  There's a problem if one of the following happened:
            //
            //  1) We overflowed when accounting for for consumed input buffer
            //  2) We wrapped when advancing SourceEntry
            //  3) PointerResult is not within InputSystemBuffer
            //

            if (!NT_SUCCESS( Status ) ||
                (PointerResult < (PVOID)SourceEntry) ||
                (PointerResult < Add2Ptr( InputSystemBuffer, sizeof(USN_RECORD) ))) {

                FLT_ASSERT( FALSE );

                Status = STATUS_INVALID_USER_BUFFER;
                break;
            }

            //
            //  We verified earlier that adding EntryLength to *OutputBufferWritten
            //  would not overflow our output buffer, meaning advancing DestEntry
            //  by EntryLength bytes is also safe.
            //

            *OutputBufferWritten += EntryLength;

            DestEntry = Add2Ptr( DestEntry, EntryLength );

            SourceEntry = (PUSN_RECORD)PointerResult;

            //
            //  If we've just advanced our next location beyond the end of the
            //  input buffer, or there isn't enough room in it for even a USN_RECORD
            //  structure, terminate the loop by setting SourceEntry to NULL so
            //  we can at least return the valid entries we have.
            //

            FLT_ASSERT( *InputBufferConsumed <= InputBufferLength );

            if ((*InputBufferConsumed >= InputBufferLength) ||
                (*InputBufferConsumed + FIELD_OFFSET( USN_RECORD, FileName ) > InputBufferLength)) {

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

    return Status;
}

FLT_POSTOP_CALLBACK_STATUS
NcPostReadFileUsnData (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:


Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure
        containing opaque handles to this filter, instance, its
        associated volume and file object.

    CompletionContext - The context for the completion routine for
        this operation.  We set this to the file object we are enumerating
        under for find files by SID.

    Flags - The flags for this operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NTSTATUS Status;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;

    //
    //  The user's buffer that we will be writing to, and the system copy
    //  of the data returned by the file system which we will read from.
    //

    PUSN_RECORD DestBuffer = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBuffer;
    PUSN_RECORD SourceBuffer = NULL;

    //
    //  The opened path's relation to the mappings.
    //

    NC_PATH_OVERLAP UserOverlap;

    //
    //  Size returned from the filesystem to us, size of the user's buffer
    //  that we can legitimately write to, the amount we actually wrote into
    //  the user's buffer, and the amount of the filesystem's buffer we have
    //  processed so far.
    //

    ULONG SizeActuallyReturned = (ULONG)Data->IoStatus.Information;
    ULONG BufferSize = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength;
    ULONG InputConsumed;
    ULONG SizeWeReturn;

    //
    //  File IDs corresponding to the parent of the user mapping, and
    //  parent of the real mapping.
    //

    ULONGLONG RealMappingParentId;
    ULONGLONG UserMappingParentId;

    OBJECT_ATTRIBUTES MappingParentAttributes;
    HANDLE MappingParentHandle = NULL;
    PFILE_OBJECT MappingParentFileObject = NULL;
    IO_STATUS_BLOCK MappingParentStatusBlock;

    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  If the operation has already failed, we're not interested in attempting
    //  to process it.
    //

    if (!NT_SUCCESS( Data->IoStatus.Status )) {

        Status = Data->IoStatus.Status;
        goto NcPostReadFileUsnDataCleanup;
    }

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadFileUsnDataCleanup;
    }

    FLT_ASSERT( BufferSize >= SizeActuallyReturned );
    if (BufferSize < sizeof( USN_RECORD ) ||
        SizeActuallyReturned < sizeof( USN_RECORD )) {

        Status = STATUS_BUFFER_TOO_SMALL;
        goto NcPostReadFileUsnDataCleanup;
    }

    //
    //  Get the file handle's name.  We are only interested in
    //  processing calls destined to the mapping.
    //

    Status = NcGetFileNameInformation( Data,
                                       NULL,
                                       NULL,
                                       FLT_FILE_NAME_OPENED |
                                           FLT_FILE_NAME_QUERY_DEFAULT |
                                           FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                       &FileInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadFileUsnDataCleanup;
    }

    Status = FltParseFileNameInformation( FileInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadFileUsnDataCleanup;
    }

    NcComparePath( &FileInfo->Name,
                   &InstanceContext->Mapping.UserMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &UserOverlap );

    if (!UserOverlap.Match) {

        Status = STATUS_SUCCESS;
        goto NcPostReadFileUsnDataCleanup;
    }

    //
    //  If another driver has posted the request, we may need to map a
    //  system buffer for the call from its Mdl.  If not, we can use the
    //  user address passed in to the call.  Be sure to check the "user"
    //  address really does point to user memory.
    //

    if (Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress != NULL) {
        Status = FltLockUserBuffer( Data );

        if (!NT_SUCCESS( Status )) {

            goto NcPostReadFileUsnDataCleanup;
        }

        DestBuffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress,
                                                   NormalPagePriority | MdlMappingNoExecute );

        if (DestBuffer == NULL) {

            Status = STATUS_NO_MEMORY;
            goto NcPostReadFileUsnDataCleanup;
        }

    } else {

        try {

            if (Data->RequestorMode != KernelMode) {
                ProbeForWrite( DestBuffer, BufferSize, sizeof( UCHAR ));
            }

        } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

            Status = STATUS_INVALID_USER_BUFFER;
        }

        if (!NT_SUCCESS( Status )) {

            goto NcPostReadFileUsnDataCleanup;
        }
    }

    //
    //  Open the mapping parents and query IDs.
    //

    InitializeObjectAttributes( &MappingParentAttributes,
                                &InstanceContext->Mapping.RealMapping.LongNamePath.ParentPath,
                                OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                NULL,
                                NULL);

    Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                 FltObjects->Instance,                 // Instance
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
                                 FltObjects->FileObject );             // Transaction info.

    if (!NT_SUCCESS( Status )) {

        FLT_ASSERT( Status != STATUS_OBJECT_PATH_NOT_FOUND &&
                    Status != STATUS_OBJECT_NAME_NOT_FOUND );

        goto NcPostReadFileUsnDataCleanup;
    }

    Status = FltQueryInformationFile( FltObjects->Instance,
                                      MappingParentFileObject,
                                      &RealMappingParentId,
                                      sizeof(RealMappingParentId),
                                      FileInternalInformation,
                                      NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadFileUsnDataCleanup;
    }

    FltClose( MappingParentHandle );
    ObDereferenceObject( MappingParentFileObject );

    MappingParentHandle = NULL;
    MappingParentFileObject = NULL;

    InitializeObjectAttributes( &MappingParentAttributes,
                                &InstanceContext->Mapping.UserMapping.LongNamePath.ParentPath,
                                OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                NULL,
                                NULL);

    Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                 FltObjects->Instance,                 // Instance
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
                                 FltObjects->FileObject );             // Transaction info.

    if (!NT_SUCCESS( Status )) {

        FLT_ASSERT( Status != STATUS_OBJECT_PATH_NOT_FOUND &&
                    Status != STATUS_OBJECT_NAME_NOT_FOUND );

        goto NcPostReadFileUsnDataCleanup;
    }

    Status = FltQueryInformationFile( FltObjects->Instance,
                                      MappingParentFileObject,
                                      &UserMappingParentId,
                                      sizeof(UserMappingParentId),
                                      FileInternalInformation,
                                      NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadFileUsnDataCleanup;
    }

    FltClose( MappingParentHandle );
    ObDereferenceObject( MappingParentFileObject );

    MappingParentHandle = NULL;
    MappingParentFileObject = NULL;

    //
    //  Allocate a new buffer and copy the contents.  Note that this is
    //  particularly important with this call, since it's not system
    //  buffered; the contents are free to change underneath us.  This
    //  allocation protects us against that, but we still must be
    //  paranoid touching the buffer, since we cannot trust that it has
    //  any integrity at this point.
    //

    SourceBuffer = ExAllocatePoolZero( PagedPool,
                                       SizeActuallyReturned,
                                       NC_TAG );

    if (SourceBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcPostReadFileUsnDataCleanup;
    }

    try {

        RtlCopyMemory( SourceBuffer, DestBuffer, SizeActuallyReturned );

    } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

        Status = STATUS_INVALID_USER_BUFFER;
        goto NcPostReadFileUsnDataCleanup;
    }

    Status = NcUsnTranslateBuffers( InstanceContext,
                                    IgnoreCase,
                                    RealMappingParentId,
                                    UserMappingParentId,
                                    SourceBuffer,
                                    DestBuffer,
                                    SizeActuallyReturned,
                                    BufferSize,
                                    &InputConsumed,
                                    &SizeWeReturn );

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadFileUsnDataCleanup;
    }

    if (InputConsumed == 0) {
        FLT_ASSERT( SizeWeReturn == 0 );
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    FLT_ASSERT( InputConsumed == SizeActuallyReturned );

    Data->IoStatus.Information = SizeWeReturn;

NcPostReadFileUsnDataCleanup:

    if (SourceBuffer != NULL) {

        ExFreePoolWithTag( SourceBuffer, NC_TAG );
    }

    if (FileInfo != NULL) {

        FltReleaseFileNameInformation( FileInfo );
    }

    if (MappingParentHandle != NULL) {

        FltClose( MappingParentHandle );
    }

    if (MappingParentFileObject != NULL) {

        ObDereferenceObject( MappingParentFileObject );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (!NT_SUCCESS( Status )) {
        Data->IoStatus.Status = Status;
        Data->IoStatus.Information = 0;
    }

    UNREFERENCED_PARAMETER( Flags );

    return FLT_POSTOP_FINISHED_PROCESSING;

}


FLT_POSTOP_CALLBACK_STATUS
NcPostEnumUsnData (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:


Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure
        containing opaque handles to this filter, instance, its
        associated volume and file object.

    CompletionContext - The context for the completion routine for
        this operation.  We set this to the file object we are enumerating
        under for find files by SID.

    Flags - The flags for this operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NTSTATUS Status;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    //
    //  The user's buffer that we will be writing to, and the system copy
    //  of the data returned by the file system which we will read from.
    //

    PUSN_RECORD DestBuffer = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBuffer;
    PUSN_RECORD SourceBuffer = NULL;

    //
    //  Size returned from the filesystem to us, size of the user's buffer
    //  that we can legitimately write to, the amount we actually wrote into
    //  the user's buffer, and the amount of the filesystem's buffer we have
    //  processed so far.
    //

    ULONG SizeActuallyReturned = (ULONG)Data->IoStatus.Information;
    ULONG BufferSize = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength;
    ULONG InputConsumed;
    ULONG SizeWeReturn;

    //
    //  File IDs corresponding to the parent of the user mapping, and
    //  parent of the real mapping.
    //

    ULONGLONG RealMappingParentId;
    ULONGLONG UserMappingParentId;

    OBJECT_ATTRIBUTES MappingParentAttributes;
    HANDLE MappingParentHandle = NULL;
    PFILE_OBJECT MappingParentFileObject = NULL;
    IO_STATUS_BLOCK MappingParentStatusBlock;

    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  If the operation has already failed, we're not interested in attempting
    //  to process it.
    //

    if (!NT_SUCCESS( Data->IoStatus.Status )) {

        Status = Data->IoStatus.Status;
        goto NcPostEnumUsnDataCleanup;
    }

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        goto NcPostEnumUsnDataCleanup;
    }

    FLT_ASSERT( BufferSize >= SizeActuallyReturned );
    if (BufferSize < sizeof( USN_RECORD ) ||
        SizeActuallyReturned < sizeof( USN_RECORD )) {

        Status = STATUS_BUFFER_TOO_SMALL;
        goto NcPostEnumUsnDataCleanup;
    }

    //
    //  If another driver has posted the request, we may need to map a
    //  system buffer for the call from its Mdl.  If not, we can use the
    //  user address passed in to the call.  Be sure to check the "user"
    //  address really does point to user memory.
    //

    if (Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress != NULL) {
        Status = FltLockUserBuffer( Data );

        if (!NT_SUCCESS( Status )) {

            goto NcPostEnumUsnDataCleanup;
        }

        DestBuffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress,
                                                   NormalPagePriority | MdlMappingNoExecute );

        if (DestBuffer == NULL) {

            Status = STATUS_NO_MEMORY;
            goto NcPostEnumUsnDataCleanup;
        }

    } else {

        try {

            if (Data->RequestorMode != KernelMode) {
                ProbeForWrite( DestBuffer, BufferSize, sizeof( UCHAR ));
            }

        } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

            Status = STATUS_INVALID_USER_BUFFER;
        }

        if (!NT_SUCCESS( Status )) {

            goto NcPostEnumUsnDataCleanup;
        }
    }

    //
    //  Open the mapping parents and query IDs.
    //

    InitializeObjectAttributes( &MappingParentAttributes,
                                &InstanceContext->Mapping.RealMapping.LongNamePath.ParentPath,
                                OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                NULL,
                                NULL);

    Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                 FltObjects->Instance,                 // Instance
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
                                 FltObjects->FileObject );             // Transaction info.

    if (!NT_SUCCESS( Status )) {

        FLT_ASSERT( Status != STATUS_OBJECT_PATH_NOT_FOUND &&
                    Status != STATUS_OBJECT_NAME_NOT_FOUND );

        goto NcPostEnumUsnDataCleanup;
    }

    Status = FltQueryInformationFile( FltObjects->Instance,
                                      MappingParentFileObject,
                                      &RealMappingParentId,
                                      sizeof(RealMappingParentId),
                                      FileInternalInformation,
                                      NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcPostEnumUsnDataCleanup;
    }

    FltClose( MappingParentHandle );
    ObDereferenceObject( MappingParentFileObject );

    MappingParentHandle = NULL;
    MappingParentFileObject = NULL;

    InitializeObjectAttributes( &MappingParentAttributes,
                                &InstanceContext->Mapping.UserMapping.LongNamePath.ParentPath,
                                OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                NULL,
                                NULL);

    Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                 FltObjects->Instance,                 // Instance
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
                                 FltObjects->FileObject );             // Transaction info.

    if (!NT_SUCCESS( Status )) {

        FLT_ASSERT( Status != STATUS_OBJECT_PATH_NOT_FOUND &&
                    Status != STATUS_OBJECT_NAME_NOT_FOUND );

        goto NcPostEnumUsnDataCleanup;
    }

    Status = FltQueryInformationFile( FltObjects->Instance,
                                      MappingParentFileObject,
                                      &UserMappingParentId,
                                      sizeof(UserMappingParentId),
                                      FileInternalInformation,
                                      NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcPostEnumUsnDataCleanup;
    }

    FltClose( MappingParentHandle );
    ObDereferenceObject( MappingParentFileObject );

    MappingParentHandle = NULL;
    MappingParentFileObject = NULL;

    //
    //  Allocate a new buffer and copy the contents.  Note that this is
    //  particularly important with this call, since it's not system
    //  buffered; the contents are free to change underneath us.  This
    //  allocation protects us against that, but we still must be
    //  paranoid touching the buffer, since we cannot trust that it has
    //  any integrity at this point.
    //

    SourceBuffer = ExAllocatePoolZero( PagedPool,
                                       SizeActuallyReturned,
                                       NC_TAG );

    if (SourceBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcPostEnumUsnDataCleanup;
    }

    try {

        RtlCopyMemory( SourceBuffer, DestBuffer, SizeActuallyReturned );

    } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

        Status = STATUS_INVALID_USER_BUFFER;
        goto NcPostEnumUsnDataCleanup;
    }

    Status = NcUsnTranslateBuffers( InstanceContext,
                                    IgnoreCase,
                                    RealMappingParentId,
                                    UserMappingParentId,
                                    Add2Ptr( SourceBuffer, sizeof(USN) ),
                                    Add2Ptr( DestBuffer, sizeof(USN) ),
                                    SizeActuallyReturned - sizeof(USN),
                                    BufferSize - sizeof(USN),
                                    &InputConsumed,
                                    &SizeWeReturn );

    if (!NT_SUCCESS( Status )) {

        goto NcPostEnumUsnDataCleanup;
    }

    if (InputConsumed == 0) {
        FLT_ASSERT( SizeWeReturn == 0 );
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    FLT_ASSERT( InputConsumed == SizeActuallyReturned - sizeof( USN ));

    Data->IoStatus.Information = SizeWeReturn + sizeof(USN);

NcPostEnumUsnDataCleanup:

    if (SourceBuffer != NULL) {

        ExFreePoolWithTag( SourceBuffer, NC_TAG );
    }

    if (MappingParentHandle != NULL) {

        FltClose( MappingParentHandle );
    }

    if (MappingParentFileObject != NULL) {

        ObDereferenceObject( MappingParentFileObject );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (!NT_SUCCESS( Status )) {
        Data->IoStatus.Status = Status;
        Data->IoStatus.Information = 0;
    }

    UNREFERENCED_PARAMETER( Flags );

    return FLT_POSTOP_FINISHED_PROCESSING;

}

VOID
NcPostReadUsnJournalWorker (
    _In_ PFLT_GENERIC_WORKITEM WorkItem,
    _In_ PFLT_FILTER Filter,
    _In_ PVOID Context
    )
/*++

Routine Description:


Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure
        containing opaque handles to this filter, instance, its
        associated volume and file object.

    CompletionContext - The context for the completion routine for
        this operation.  We set this to the file object we are enumerating
        under for find files by SID.

    Flags - The flags for this operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    PFLT_CALLBACK_DATA Data = (PFLT_CALLBACK_DATA)Context;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NTSTATUS Status;
    BOOLEAN IgnoreCase = !BooleanFlagOn( Data->Iopb->TargetFileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    //
    //  The user's buffer that we will be writing to, and the system copy
    //  of the data returned by the file system which we will read from.
    //

    PUSN_RECORD DestBuffer = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBuffer;
    PUSN_RECORD SourceBuffer = NULL;

    //
    //  Size returned from the filesystem to us, size of the user's buffer
    //  that we can legitimately write to, the amount we actually wrote into
    //  the user's buffer, and the amount of the filesystem's buffer we have
    //  processed so far.
    //

    ULONG SizeActuallyReturned = (ULONG)Data->IoStatus.Information;
    ULONG BufferSize = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength;
    ULONG InputConsumed;
    ULONG SizeWeReturn;

    //
    //  File IDs corresponding to the parent of the user mapping, and
    //  parent of the real mapping.
    //

    ULONGLONG RealMappingParentId;
    ULONGLONG UserMappingParentId;

    OBJECT_ATTRIBUTES MappingParentAttributes;
    HANDLE MappingParentHandle = NULL;
    PFILE_OBJECT MappingParentFileObject = NULL;
    IO_STATUS_BLOCK MappingParentStatusBlock;

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  If the operation has already failed, we're not interested in attempting
    //  to process it.
    //

    if (!NT_SUCCESS( Data->IoStatus.Status )) {

        Status = Data->IoStatus.Status;
        goto NcPostReadUsnJournalSafeCleanup;
    }

    Status = FltGetInstanceContext( Data->Iopb->TargetInstance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadUsnJournalSafeCleanup;
    }

    FLT_ASSERT( BufferSize >= SizeActuallyReturned );
    if (BufferSize < sizeof( USN_RECORD ) + sizeof( USN )||
        SizeActuallyReturned < sizeof( USN_RECORD ) + sizeof( USN )) {

        Status = STATUS_BUFFER_TOO_SMALL;
        goto NcPostReadUsnJournalSafeCleanup;
    }

    //
    //  We always post this request.  In future, we may skip this post
    //  in some conditions, but for now, OutputMdlAddress had better
    //  be valid.
    //

    FLT_ASSERT( Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress != NULL );
    if (Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress != NULL) {
        Status = FltLockUserBuffer( Data );

        if (!NT_SUCCESS( Status )) {

            goto NcPostReadUsnJournalSafeCleanup;
        }

        DestBuffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress,
                                                   NormalPagePriority | MdlMappingNoExecute );

        if (DestBuffer == NULL) {

            Status = STATUS_NO_MEMORY;
            goto NcPostReadUsnJournalSafeCleanup;
        }

    } else {

        try {

            if (Data->RequestorMode != KernelMode) {
                ProbeForWrite( DestBuffer, BufferSize, sizeof( UCHAR ));
            }

        } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

            Status = STATUS_INVALID_USER_BUFFER;
        }

        if (!NT_SUCCESS( Status )) {

            goto NcPostReadUsnJournalSafeCleanup;
        }
    }

    //
    //  Open the mapping parents and query IDs.
    //

    InitializeObjectAttributes( &MappingParentAttributes,
                                &InstanceContext->Mapping.RealMapping.LongNamePath.ParentPath,
                                OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                NULL,
                                NULL);

    Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                 Data->Iopb->TargetInstance,           // Instance
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
                                 Data->Iopb->TargetFileObject );       // Transaction info.

    if (!NT_SUCCESS( Status )) {

        FLT_ASSERT( Status != STATUS_OBJECT_PATH_NOT_FOUND &&
                    Status != STATUS_OBJECT_NAME_NOT_FOUND );

        goto NcPostReadUsnJournalSafeCleanup;
    }

    Status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                      MappingParentFileObject,
                                      &RealMappingParentId,
                                      sizeof(RealMappingParentId),
                                      FileInternalInformation,
                                      NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadUsnJournalSafeCleanup;
    }

    FltClose( MappingParentHandle );
    ObDereferenceObject( MappingParentFileObject );

    MappingParentHandle = NULL;
    MappingParentFileObject = NULL;

    InitializeObjectAttributes( &MappingParentAttributes,
                                &InstanceContext->Mapping.UserMapping.LongNamePath.ParentPath,
                                OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                NULL,
                                NULL);

    Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                 Data->Iopb->TargetInstance,           // Instance
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
                                 Data->Iopb->TargetFileObject );       // Transaction info.

    if (!NT_SUCCESS( Status )) {

        FLT_ASSERT( Status != STATUS_OBJECT_PATH_NOT_FOUND &&
                    Status != STATUS_OBJECT_NAME_NOT_FOUND );

        goto NcPostReadUsnJournalSafeCleanup;
    }

    Status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                      MappingParentFileObject,
                                      &UserMappingParentId,
                                      sizeof(UserMappingParentId),
                                      FileInternalInformation,
                                      NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadUsnJournalSafeCleanup;
    }

    FltClose( MappingParentHandle );
    ObDereferenceObject( MappingParentFileObject );

    MappingParentHandle = NULL;
    MappingParentFileObject = NULL;

    //
    //  Allocate a new buffer and copy the contents.  Note that this is
    //  particularly important with this call, since it's not system
    //  buffered; the contents are free to change underneath us.  This
    //  allocation protects us against that, but we still must be
    //  paranoid touching the buffer, since we cannot trust that it has
    //  any integrity at this point.
    //

    SourceBuffer = ExAllocatePoolZero( PagedPool,
                                       SizeActuallyReturned,
                                       NC_TAG );

    if (SourceBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcPostReadUsnJournalSafeCleanup;
    }

    try {

        RtlCopyMemory( SourceBuffer, DestBuffer, SizeActuallyReturned );

    } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

        Status = STATUS_INVALID_USER_BUFFER;
        goto NcPostReadUsnJournalSafeCleanup;
    }

    Status = NcUsnTranslateBuffers( InstanceContext,
                                    IgnoreCase,
                                    RealMappingParentId,
                                    UserMappingParentId,
                                    Add2Ptr( SourceBuffer, sizeof(USN) ),
                                    Add2Ptr( DestBuffer, sizeof(USN) ),
                                    SizeActuallyReturned - sizeof(USN),
                                    BufferSize - sizeof(USN),
                                    &InputConsumed,
                                    &SizeWeReturn );

    if (!NT_SUCCESS( Status )) {

        goto NcPostReadUsnJournalSafeCleanup;
    }

    if (InputConsumed == 0) {
        FLT_ASSERT( SizeWeReturn == 0 );
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    FLT_ASSERT( InputConsumed == SizeActuallyReturned  - sizeof( USN ));

    Data->IoStatus.Information = SizeWeReturn + sizeof(USN);

NcPostReadUsnJournalSafeCleanup:

    if (SourceBuffer != NULL) {

        ExFreePoolWithTag( SourceBuffer, NC_TAG );
    }

    if (MappingParentHandle != NULL) {

        FltClose( MappingParentHandle );
    }

    if (MappingParentFileObject != NULL) {

        ObDereferenceObject( MappingParentFileObject );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (!NT_SUCCESS( Status )) {
        Data->IoStatus.Status = Status;
        Data->IoStatus.Information = 0;
    }

    FltCompletePendedPostOperation( Data );

    FltFreeGenericWorkItem( WorkItem );

    UNREFERENCED_PARAMETER( Filter );

}

FLT_POSTOP_CALLBACK_STATUS
NcPostReadUsnJournal (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    NTSTATUS Status;
    PFLT_GENERIC_WORKITEM WorkItem;

    //
    //  Lock the user's buffer so we can post this request.
    //

    Status = FltLockUserBuffer( Data );
    if (!NT_SUCCESS( Status )) {

        Data->IoStatus.Status = Status;
        Data->IoStatus.Information = 0;
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  Allocate a workitem and post.  At this point, we will typically
    //  have TopLevelIrp set (along with filesystem locks.)  We don't want
    //  to issue creates back into the filesystem from here.  Note that
    //  we can get away with this because the filesystem will not wait -
    //  it has completed this request, and will go on from here.
    //

    WorkItem = FltAllocateGenericWorkItem();

    if (WorkItem == NULL) {

        Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Data->IoStatus.Information = 0;
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    Status = FltQueueGenericWorkItem( WorkItem,
                                      Data->Iopb->TargetInstance,
                                      NcPostReadUsnJournalWorker,
                                      CriticalWorkQueue,
                                      Data );

    if (!NT_SUCCESS( Status )) {

        Data->IoStatus.Status = Status;
        Data->IoStatus.Information = 0;
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    return FLT_POSTOP_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );
}

