/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    ncdirenum.c

Abstract:

    Contains routines to process user-initiated directory enumerations.
    Depending on path, we may need to suppress the real mapping from
    being visible to the user, or "inject" the user mapping for the
    user.  We must take care to do so having regard for the pattern
    matching and case sensitivity dictated by the caller.

Environment:

    Kernel mode

--*/

#include "nc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcCopyDirEnumEntry)
#pragma alloc_text(PAGE, NcDirEnumSelectNextEntry)
#pragma alloc_text(PAGE, NcEnumerateDirectory)
#pragma alloc_text(PAGE, NcEnumerateDirectorySetupInjection)
#pragma alloc_text(PAGE, NcEnumerateDirectoryReset)
#pragma alloc_text(PAGE, NcPopulateCacheEntry)
#pragma alloc_text(PAGE, NcSkipName)
#pragma alloc_text(PAGE, NcStreamHandleContextEnumClose)
#pragma alloc_text(PAGE, NcStreamHandleContextEnumSetup)
#pragma alloc_text(PAGE, NcStreamHandleContextDirEnumCreate)
#endif

FLT_PREOP_CALLBACK_STATUS
NcEnumerateDirectory (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Routine is invoked when a directory enumeration is issued by the
    user.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    //TODO WE SHOULD CONSIDER MOVING THIS TO POST BECAUSE NTFS WILL TAKE CARE
    //     OF SYNC.
    FLT_PREOP_CALLBACK_STATUS ReturnValue;
    NTSTATUS Status;

    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    PNC_STREAM_HANDLE_CONTEXT HandleContext = NULL;
    PNC_DIR_QRY_CONTEXT DirCtx = NULL;

    PFLT_FILE_NAME_INFORMATION FileNameInformation = NULL;

    NC_PATH_OVERLAP RealOverlap;
    NC_PATH_OVERLAP UserOverlap;

    BOOLEAN Reset = BooleanFlagOn( Data->Iopb->OperationFlags, SL_RESTART_SCAN );
    BOOLEAN FirstQuery;
    BOOLEAN Single = BooleanFlagOn( Data->Iopb->OperationFlags, SL_RETURN_SINGLE_ENTRY );
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );
    
    FILE_INFORMATION_CLASS InformationClass = 
        Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;

    PVOID UserBuffer;
    ULONG BufferSize; //size for user and system buffers.

    BOOLEAN Unlock = FALSE;

    //Vars for moving data into user buffer.
    ULONG NumEntriesCopied;
    ULONG UserBufferOffset;
    ULONG LastEntryStart;
    BOOLEAN MoreRoom;
    PNC_CACHE_ENTRY NextEntry;

    DIRECTORY_CONTROL_OFFSETS Offsets;

    BOOLEAN FoundStructureOffsets;

    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    FoundStructureOffsets = NcDetermineStructureOffsets( &Offsets,
                                                         InformationClass );

    if (!FoundStructureOffsets) {

        Status = STATUS_INVALID_PARAMETER;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcEnumerateDirectoryCleanup;
    }

    //
    //  Get our instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcEnumerateDirectoryCleanup;
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
        goto NcEnumerateDirectoryCleanup;
    }

    Status = FltParseFileNameInformation( FileNameInformation );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcEnumerateDirectoryCleanup;
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

    if (!(UserOverlap.Parent || RealOverlap.Parent )) {

        //
        //  We are not interested in this directory
        //  because it is not the parent of either
        //  mapping. This means we can just passthrough.
        //

        Status = STATUS_SUCCESS;
        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcEnumerateDirectoryCleanup;
    }

    Status = NcStreamHandleContextAllocAndAttach( FltObjects->Filter,
                                                  FltObjects->Instance,
                                                  FltObjects->FileObject,
                                                  &HandleContext );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcEnumerateDirectoryCleanup;
    }

    FLT_ASSERT( HandleContext != NULL );

    DirCtx = &HandleContext->DirectoryQueryContext;
    _Analysis_assume_( DirCtx != NULL );

    //
    //  Before looking at the context, we have to acquire the lock.
    //
    
    NcLockStreamHandleContext( HandleContext );
    Unlock = TRUE;

    //
    //  We don't allow multiple outstanding enumeration requests on
    //  a single handle.
    //
    //  TODO: This needs to change.
    //

    if (DirCtx->EnumerationOutstanding) {

        Status = STATUS_UNSUCCESSFUL;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcEnumerateDirectoryCleanup;

    }

    DirCtx->EnumerationOutstanding = TRUE;

    //
    //  Now drop the lock.  We're protected by the EnumerationOutstanding
    //  flag; nobody else can muck with the enumeration context structure.
    //

    NcUnlockStreamHandleContext( HandleContext );
    Unlock = FALSE;

    //
    //  Now we need to initialize or clear the cache and query options.
    //
    
    Status = NcStreamHandleContextEnumSetup( DirCtx,
                                             InstanceContext,
                                             &Offsets,
                                             Data,
                                             FltObjects,
                                             UserOverlap,
                                             &FirstQuery );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcEnumerateDirectoryCleanup;
    }

    //
    //  Prepare to populate the user buffer.
    //

    UserBuffer = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

    BufferSize = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length;

    //
    //  Lets copy data into the user buffer.
    //

    NumEntriesCopied = 0;
    UserBufferOffset = 0;

    do {

        //
        //  If there is no cache entry, populate it.
        //

        if (DirCtx->Cache.Buffer == NULL) {

            Status = NcPopulateCacheEntry( FltObjects->Instance,
                                           FltObjects->FileObject,
                                           Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length,
                                           Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass,
                                           Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName,
                                           Reset,
                                           &DirCtx->Cache);

            //
            //  We only want to reset the cache once.
            //

            Reset = FALSE;

            //
            //  There was a problem populating cache, pass up to user.
            //

            if (!NT_SUCCESS( Status )) {

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcEnumerateDirectoryCleanup;
            }

        }

        NextEntry = NcDirEnumSelectNextEntry( DirCtx, &Offsets, IgnoreCase );

        if (NextEntry == NULL) {

            //
            //  There are no more entries.
            //

            break;
        }

        if (NcSkipName( &Offsets, 
                        DirCtx, 
                        RealOverlap, 
                        &InstanceContext->Mapping,
                        IgnoreCase )) {

            //
            //  This entry is the real mapping path. That means we have to mask it...
            //  We will say there is more room and continue.
            //

            MoreRoom = TRUE;

        } else {

            //
            //  We are keeping this entry!
            //

            try {

                LastEntryStart = UserBufferOffset;
                UserBufferOffset = NcCopyDirEnumEntry( UserBuffer,
                                                       UserBufferOffset,
                                                       BufferSize,
                                                       NextEntry,
                                                       &Offsets,
                                                       &MoreRoom );

            } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

                Status = STATUS_INVALID_USER_BUFFER;
                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcEnumerateDirectoryCleanup;
            }

            if (MoreRoom) {

                NumEntriesCopied++;
            }

        }// end of "we are copying entry"

    } while (MoreRoom && 
             (Single ? (NumEntriesCopied < 1) : TRUE));

    if (NumEntriesCopied > 0) {

        //
        //  Now we know what the last entry in the user buffer is going to be.
        //  Set its NextEntryOffset to 0, so that the user knows its the last element.
        //

        try {

            NcSetNextEntryOffset( Add2Ptr(UserBuffer, LastEntryStart),
                                  &Offsets, 
                                  TRUE );

        } except (NcExceptionFilter( GetExceptionInformation(), TRUE )) {

            Status = STATUS_INVALID_USER_BUFFER;
            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcEnumerateDirectoryCleanup;
        }
    }

    //
    //  We finished copying data.
    //

    ReturnValue = FLT_PREOP_COMPLETE;

    if (NumEntriesCopied == 0) {

        if (FirstQuery) {

            Status = STATUS_NO_SUCH_FILE;

        } else {

            Status = STATUS_NO_MORE_FILES;
        }

    } else {

        Status = STATUS_SUCCESS;
    }

    ReturnValue = FLT_PREOP_COMPLETE;
    goto NcEnumerateDirectoryCleanup;

NcEnumerateDirectoryCleanup:

    if (ReturnValue == FLT_PREOP_COMPLETE) {
 
        //
        //  We need to write back results of query.
        //
 
        Data->IoStatus.Status = Status;
 
        if (NT_SUCCESS( Status )) {
 
            //success
            Data->IoStatus.Information = UserBufferOffset;

        } else {
 
            //failure
            Data->IoStatus.Information = 0;
        }
    }
 
    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }
 
    if (DirCtx != NULL) {
        
        if (!Unlock) {
            NcLockStreamHandleContext( HandleContext );
            Unlock = TRUE;
        }

        FLT_ASSERT( DirCtx->EnumerationOutstanding );
        DirCtx->EnumerationOutstanding = FALSE;

        NcUnlockStreamHandleContext( HandleContext );
        Unlock = FALSE;

        FltReleaseContext( HandleContext );
    }

    FLT_ASSERT( !Unlock );
 
    if (FileNameInformation != NULL) {

        FltReleaseFileNameInformation( FileNameInformation );
    }
 
    return ReturnValue;
}

NTSTATUS 
NcEnumerateDirectorySetupInjection (
    _Inout_ PNC_DIR_QRY_CONTEXT DirQryCtx,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ FILE_INFORMATION_CLASS InformationClass
    )
/*++

Routine Description:

    Sets up directory enumeration context cache so that we are ready to
    perform injection.

Arguments:

    DirQryCtx - Pointer to directory query context (on the stream handle.)

    FltObjects - FltObjects structure for this operation.
    
    InstanceContext - Instance Context for this operation.

    Offsets - Offsets structure for this information class.

    InformationClass - The information class for this operation.

Return Value:

    Returns STATUS_SUCCESS on success, otherwise an appropriate error code.

--*/
{
    NTSTATUS Status;

    OBJECT_ATTRIBUTES RealParentAttributes;
    HANDLE RealParentHandle = 0; //close always
    PFILE_OBJECT RealParentFileObj = NULL;
    IO_STATUS_BLOCK RealParentStatusBlock;
    char * QueryBuffer = NULL; //free on error, when no injection
    ULONG QueryBufferLength = 0;
    USHORT NameLength;
    ULONG QueryBufferLengthRead;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    PAGED_CODE();


    //
    //  If the user has specified a search string, and if our user mapping
    //  should not be returned in this search string, return success.  We
    //  don't need to inject anything.
    //

    if (DirQryCtx->SearchString.Length > 0 &&
        !FsRtlIsNameInExpression( &DirQryCtx->SearchString,
                                  &InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName,
                                  IgnoreCase,
                                  NULL ) &&
        !FsRtlIsNameInExpression( &DirQryCtx->SearchString,
                                  &InstanceContext->Mapping.UserMapping.ShortNamePath.FinalComponentName,
                                  IgnoreCase,
                                  NULL )) {

        Status = STATUS_SUCCESS;
        goto NcEnumerateDirectorySetupCleanup;
    }

    //
    //  Initialize insertion info.
    //
    //  We have to insert the final component of the real mapping
    //  as the final component of the user mapping. To do this we
    //  will open the parent of the real mapping, and query the real
    //  mapping.
    //  Then we will overwrite the real mapping's name with
    //  the final component of the user mapping. This data will be
    //  stored in the DirQryCtx for later injection.
    //

    //
    //  Open parent of real mapping.
    //

    InitializeObjectAttributes( &RealParentAttributes,
                                &InstanceContext->Mapping.RealMapping.LongNamePath.ParentPath,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);

    Status = NcCreateFileHelper( NcGlobalData.FilterHandle,            // Filter
                                 FltObjects->Instance,                 // InstanceOffsets
                                 &RealParentHandle,                    // Returned Handle
                                 &RealParentFileObj,                   // Returned FileObject
                                 FILE_LIST_DIRECTORY|FILE_TRAVERSE,    // Desired Access
                                 &RealParentAttributes,                // object attributes
                                 &RealParentStatusBlock,               // Returned IOStatusBlock
                                 0,                                    // Allocation Size
                                 FILE_ATTRIBUTE_NORMAL,                // File Attributes
                                 0,                                    // Share Access
                                 FILE_OPEN,                            // Create Disposition
                                 FILE_DIRECTORY_FILE,                  // Create Options
                                 NULL,                                 // Ea Buffer
                                 0,                                    // EA Length
                                 IO_IGNORE_SHARE_ACCESS_CHECK,         // Flags
                                 FltObjects->FileObject );             // Transaction state
   
    if (!NT_SUCCESS( Status )) {

        goto NcEnumerateDirectorySetupCleanup;
    }

    //
    //  Allocate Buffer to store mapping data.
    //

    NameLength = Max( InstanceContext->Mapping.RealMapping.LongNamePath.FinalComponentName.Length,
                      InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Length );

    QueryBufferLength = Offsets->FileNameDist + NameLength;

    QueryBuffer = ExAllocatePoolWithTag( PagedPool, QueryBufferLength, NC_DIR_QRY_CACHE_TAG );

    if (QueryBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcEnumerateDirectorySetupCleanup;
    }

    //
    //  Query the information from the parent of the real mapping.
    //

    Status = NcQueryDirectoryFile( FltObjects->Instance,
                                   RealParentFileObj,
                                   QueryBuffer,
                                   QueryBufferLength,
                                   InformationClass,
                                   TRUE,//Return single entry
                                   &InstanceContext->Mapping.RealMapping.LongNamePath.FinalComponentName,
                                   FALSE,//restart scan
                                   &QueryBufferLengthRead);

    if (Status == STATUS_NO_SUCH_FILE) {

        //
        //  The user mapping does not exist, this is allowed. It means we
        //  have nothing to inject.
        //

        DirQryCtx->InjectionEntry.Buffer = NULL;
        DirQryCtx->InjectionEntry.CurrentOffset = 0;

        ExFreePoolWithTag( QueryBuffer, NC_DIR_QRY_CACHE_TAG );
        QueryBuffer = NULL;

        Status = STATUS_SUCCESS;

    } else if (!NT_SUCCESS( Status )) {

        //
        //  An unexpected error occurred, return code.
        //

        goto NcEnumerateDirectorySetupCleanup;
        
    } else {

        //
        //  Now we have to munge the real mapping directory entry into a
        //  user mapping directory entry.
        //

        NcSetFileName( QueryBuffer,
                       InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Buffer,
                       InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Length,
                       Offsets,
                       TRUE );

        NcSetShortName( QueryBuffer,
                        InstanceContext->Mapping.UserMapping.ShortNamePath.FinalComponentName.Buffer,
                        InstanceContext->Mapping.UserMapping.ShortNamePath.FinalComponentName.Length,
                        Offsets );

        FLT_ASSERT( DirQryCtx->InjectionEntry.Buffer == NULL );

        //
        //  Set the injection entry up in the cache.
        //

        DirQryCtx->InjectionEntry.Buffer = QueryBuffer;
        DirQryCtx->InjectionEntry.CurrentOffset = 0;
    }

NcEnumerateDirectorySetupCleanup:

    if (!NT_SUCCESS( Status )) {

        if(QueryBuffer != NULL) {

            ExFreePoolWithTag( QueryBuffer, NC_DIR_QRY_CACHE_TAG );
        }

    }

    if (RealParentHandle != NULL) {

        FltClose( RealParentHandle );
    }

    if (RealParentFileObj != NULL) {

        ObDereferenceObject( RealParentFileObj );
    }

    return Status;
}

VOID
NcEnumerateDirectoryReset (
    _Inout_ PNC_DIR_QRY_CONTEXT DirCtx 
    )
/*++

Routine Description:

    Tears down any stream handle context related to enumeration and prepares
    the stream handle context for reuse.

Arguments:

    DirQryCtx - Pointer to directory query context (on the stream handle.)

Return Value:

    None.

--*/
{
    PAGED_CODE();

    ExFreePoolWithTag( DirCtx->Cache.Buffer, NC_DIR_QRY_CACHE_TAG );
    DirCtx->Cache.Buffer = NULL;
    DirCtx->Cache.CurrentOffset = 0;

    ExFreePoolWithTag( DirCtx->InjectionEntry.Buffer, NC_DIR_QRY_CACHE_TAG );
    DirCtx->InjectionEntry.Buffer = NULL;
    DirCtx->InjectionEntry.CurrentOffset = 0;
}

BOOLEAN
NcSkipName (
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ PNC_DIR_QRY_CONTEXT Context, 
    _In_ NC_PATH_OVERLAP RealOverlap,
    _In_ PNC_MAPPING Mapping,
    _In_ BOOLEAN IgnoreCase
    )
/*++

Routine Description:

    Determines if the next entry is for the real mapping.  If it is, we want
    to "skip" returning this entry and proceed to the next.

Arguments:

    Offsets - Offset information for this enumeration class.

    Context - Pointer to directory query context (on the stream handle.)
        This is used to obtain the entry we are contemplating returning.

    RealOverlap - Relationship of the object that enumeration is being
        requested on to the real mapping.  This routine will only skip
        entries if we are enumerating the parent of the real mapping.

Return Value:

    TRUE if this entry should be skipped/suppressed; FALSE if it should be
    returned to the user.

--*/
{
    BOOLEAN Result = FALSE;
    PVOID CacheEntry;
    UNICODE_STRING CacheString;
    PUNICODE_STRING IgnoreString = &Mapping->RealMapping.LongNamePath.FinalComponentName;
    ULONG ElementSize;
    BOOLEAN LastElement;

    PAGED_CODE();

    if (RealOverlap.Parent) {

        //
        //  We have to check for a match.
        //

        CacheEntry = Add2Ptr( Context->Cache.Buffer, Context->Cache.CurrentOffset);
        ElementSize = NcGetEntrySize( CacheEntry, Offsets );
        LastElement = (BOOLEAN)(NcGetNextEntryOffset( CacheEntry, Offsets ) == 0);

        CacheString.Buffer = NcGetFileName( CacheEntry, Offsets );
        CacheString.Length = (USHORT) NcGetFileNameLength( CacheEntry, Offsets );
        CacheString.MaximumLength = CacheString.Length;


        if (RtlCompareUnicodeString( &CacheString, 
                                     IgnoreString,
                                     IgnoreCase ) == 0) {

            //
            //  We need to ignore this name.
            //

            Result = TRUE;

            //
            //  skip
            //

            if (LastElement) {

                //
                //  This was the last element in the entry, so we should clean the entry.
                //

                ExFreePoolWithTag(Context->Cache.Buffer, NC_DIR_QRY_CACHE_TAG);
                Context->Cache.Buffer = NULL;
                Context->Cache.CurrentOffset = 0;

            } else {

                //
                //  Entry has more elements, update offset counter.
                //

                Context->Cache.CurrentOffset += ElementSize;
            }
        } 
    }

    return Result;
}

NTSTATUS
NcPopulateCacheEntry (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG BufferLength,
    _In_ FILE_INFORMATION_CLASS FileInfoClass,
    _In_ PUNICODE_STRING SearchString,
    _In_ BOOLEAN RestartScan,
    _Out_ PNC_CACHE_ENTRY Cache
    )
/*++

Routine Description:

    Obtains the next entry from the filesystem.  By always reading ahead, we
    can determine when to return the injected entry (if one exists) while
    attempting to preserve directory sort order.

Arguments:

    Instance - Instance of this filter in the filter stack.

    FileObject - Directory that we are enumerating.

    BufferLength - Size, in bytes, of the buffer to allocate within this
        routine.

    FileInfoClass - Directory enumeration class that we are using.

    SearchString - Pointer to the string containing the enumeration criteria.
        Will be empty if enumerating all objects in a directory.

    RestartScan - Boolean value set to TRUE if we should start enumeration from
        the beginning.  Set to FALSE to continue from the previous point.

    Cache - Pointer to our structure for receiving the cached entry.

Return Value:

    The return value is the Status of the operation.

--*/
{

    NTSTATUS Status;
    PVOID Buffer;

    PAGED_CODE();

    if (SearchString != NULL && SearchString->Buffer == NULL) {

        //
        //  In the case there is no search string provided,
        //  don't pass one to the filesystem.
        //

        SearchString = NULL;
    }

    Buffer = ExAllocatePoolWithTag( PagedPool, BufferLength, NC_DIR_QRY_CACHE_TAG );

    if (Buffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    Status = NcQueryDirectoryFile( Instance,
                                   FileObject,
                                   Buffer,
                                   BufferLength,
                                   FileInfoClass,
                                   FALSE,
                                   SearchString,
                                   RestartScan,
                                   NULL);

    if (Status == STATUS_NO_MORE_FILES || Status == STATUS_NO_SUCH_FILE) {

        //
        //  There are no more files. Keep cache empty.
        //

        Cache->Buffer = NULL;
        Cache->CurrentOffset = 0;

        ExFreePoolWithTag( Buffer, NC_DIR_QRY_CACHE_TAG );

        Status = STATUS_SUCCESS;

    } else if (NT_SUCCESS( Status )) {

        //
        //  There were entries, populate.
        //

        Cache->Buffer = Buffer;
        Cache->CurrentOffset = 0;

    } else {

        //
        //  An unspecified error occurred, return it.
        //

        ExFreePoolWithTag( Buffer, NC_DIR_QRY_CACHE_TAG );
    }

    return Status;
}

PNC_CACHE_ENTRY
NcDirEnumSelectNextEntry( 
    _Inout_ PNC_DIR_QRY_CONTEXT Context, 
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ BOOLEAN IgnoreCase
    )
/*++

Routine Description:

    This routine determines whether the cached entry (returned from the
    filesystem) should be returned now or whether the injected entry (as a
    result of our mapping) should be returned now.

Arguments:

    Context - The enumeration context of this handle.

    Offsets - Information describing the offsets for this enumeration class.

    IgnoreCase - TRUE if we are case insensitive, FALSE if case sensitive.

Return Value:

    A pointer to the entry we should return, or NULL if there is nothing
    remaining to return.

--*/
{
    PNC_CACHE_ENTRY NextEntry;
    UNICODE_STRING CacheString;
    UNICODE_STRING InsertString;
    PVOID CacheEntry;
    PVOID InjectEntry;

    PAGED_CODE();

    //
    //  Figure out which name comes first
    //

    if ((Context->Cache.Buffer == NULL) && 
        (Context->InjectionEntry.Buffer == NULL)) {

        //
        //  There are no names left, return STATUS_NO_MORE_FILES
        //

        NextEntry = NULL;

    } else if (Context->Cache.Buffer == NULL) {

        //
        //  The cache is empty, so inject.
        //

        NextEntry = &Context->InjectionEntry;

    } else if (Context->InjectionEntry.Buffer == NULL) {

        //
        //  The injection entry is empty, drain the cache.
        //

        NextEntry = &Context->Cache;

    } else {

        //
        //  We have to seek in the entry buffer to the current entry within the buffer.
        //

        CacheEntry = Add2Ptr( Context->Cache.Buffer, Context->Cache.CurrentOffset );
        InjectEntry = Add2Ptr( Context->InjectionEntry.Buffer, Context->InjectionEntry.CurrentOffset );

        //
        //  Find names within the entries.
        //

        CacheString.Buffer = NcGetFileName( CacheEntry, Offsets );
        CacheString.Length = (USHORT) NcGetFileNameLength( CacheEntry, Offsets );
        CacheString.MaximumLength = CacheString.Length;

        InsertString.Buffer = NcGetFileName( InjectEntry, Offsets );
        InsertString.Length = (USHORT) NcGetFileNameLength( InjectEntry, Offsets );
        InsertString.MaximumLength = InsertString.Length;

        //
        //  Compare the names
        //

        if (RtlCompareUnicodeString( &CacheString,
                                     &InsertString,
                                     IgnoreCase ) < 0) {

            //
            //  Cache string comes first
            //

            NextEntry = &Context->Cache;

        } else {

            //
            //  insert string comes first
            //

            NextEntry = &Context->InjectionEntry;
        }
    }
    return NextEntry;
}

_Success_(*Copied)
ULONG
NcCopyDirEnumEntry (
    _Out_ PVOID UserBuffer,
    _In_ ULONG UserOffset,
    _In_ ULONG UserSize,
    _Inout_ PNC_CACHE_ENTRY Entry,
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _Out_ PBOOLEAN Copied 
    )
/*++

Routine Description:

    This routine copies a single enumeration result into the caller's
    buffer.

Arguments:

    UserBuffer - Pointer to the caller's buffer.

    UserOffset - Offset within the caller's buffer that we intend to write new
        results.

    UserSize - Size of the caller's buffer, in bytes.

    Entry - Pointer to the directory entry that we intend to return.

    Offsets - Information describing the offsets for this enumeration class.

    Copied - Pointer to a boolean value indicating whether this routine copied
        a new entry or not.

Return Value:

    The new offset in the user buffer that any future copies should use.

--*/
{
    PVOID Element =        Add2Ptr( Entry->Buffer, Entry->CurrentOffset );
    PVOID Dest =           Add2Ptr( UserBuffer, UserOffset );
    ULONG ElementSize =    NcGetEntrySize( Element, Offsets );
    BOOLEAN LastElement =  (BOOLEAN)(NcGetNextEntryOffset( Element, Offsets ) == 0);

    PAGED_CODE();

    if (UserSize - UserOffset >= ElementSize) {

        //
        //  There is enough room for this element, so copy it.
        //

        RtlCopyMemory( Dest, Element, ElementSize );
        UserOffset += ElementSize;
        *Copied = TRUE;

        //
        //  Update Entry's Offset
        //

        if (LastElement) {

            //
            //  This was the last element in the entry, so we should clean the
            //  entry.
            //

            ExFreePoolWithTag( Entry->Buffer, NC_TAG );
            Entry->Buffer = NULL;
            Entry->CurrentOffset = 0;

            //
            //  The last element in cached entries have a NextEntryOffset of 0,
            //  make sure that we report the actual next entry offset.
            //

            NcSetNextEntryOffset( Dest, Offsets, FALSE );

        } else {

            //
            //  Entry has more elements, update offset counter.
            //

            Entry->CurrentOffset += ElementSize;
        }

    } else {

        //
        //  User buffer does not have enough space.
        //

        *Copied = FALSE;
    }

    return UserOffset;
}

NTSTATUS 
NcStreamHandleContextDirEnumCreate ( 
    _Out_ PNC_DIR_QRY_CONTEXT Context
    )
/*++

Routine Description:

    Initializes the stream handle context ready for directory enumeration
    requests.

Arguments:

    Context - Pointer to the directory context.

Return Value:

    The return value is the Status of the operation.  Currently this operation
    cannot fail, so this function always returns STATUS_SUCCESS.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Context->EnumerationOutstanding = FALSE;
    Context->InUse = FALSE;
    Context->Cache.Buffer = NULL;
    Context->Cache.CurrentOffset = 0;
    Context->InjectionEntry.Buffer = NULL;
    Context->InjectionEntry.CurrentOffset = 0;
    Context->SearchString.Length = 0;
    Context->SearchString.MaximumLength = 0;
    Context->SearchString.Buffer = NULL;

    return Status;
}


NTSTATUS
NcStreamHandleContextEnumSetup (
    _Inout_ PNC_DIR_QRY_CONTEXT DirContext,
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ NC_PATH_OVERLAP UserMappingOverlap,
    _Out_ PBOOLEAN FirstUsage
    )
/*++

Routine Description:

    Acquires the stream handle context and initializes it for a directory
    enumeration.

Arguments:

    DirContext - Pointer to the directory context.

    InstanceContext - Pointer to this instance's context.

    Offsets - Offsets structure for this enumeration class.

    Data - Callback data for this operation.

    FltObjects - FltObjects structure for this operation.

    UserMappingOverlap - The overlap between the user mapping and this file
        object.

    FirstUsage - Weather or not this is the first usage of this handle in a
        directory enumeration.

Return Value:

    The return value is the Status of the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PUNICODE_STRING SearchString = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName;
    FILE_INFORMATION_CLASS InformationClass = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
    BOOLEAN ResetSearch = BooleanFlagOn( Data->Iopb->OperationFlags, SL_RESTART_SCAN );
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    PAGED_CODE();

    //
    //  This context could be in its first use. If it is, then we need to 
    //  setup the search string and information class. 
    //
    
    if (DirContext->InUse == FALSE) {

        //
        //  This was the first usage of the context.
        //  We should set up the search string.
        //

        if (SearchString != NULL) {

            DirContext->SearchString.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                                     SearchString->Length,
                                                                     NC_DIR_QRY_SEARCH_STRING );

            if (DirContext->SearchString.Buffer == NULL) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto NcStreamHandleContextEnumSetupCleanup;
            }
            
            DirContext->SearchString.MaximumLength = SearchString->Length;

            if (IgnoreCase) {

                RtlUpcaseUnicodeString( &DirContext->SearchString, SearchString, FALSE );

            } else {

                RtlCopyUnicodeString( &DirContext->SearchString, SearchString );
            }

            DirContext->SearchString.Length = SearchString->Length;

        } else {

            RtlInitEmptyUnicodeString( &DirContext->SearchString,
                                       NULL,
                                       0 );
        }

        DirContext->InformationClass = InformationClass;

        //
        //  We should write back to the caller so they know its the first usage.
        //

        *FirstUsage = TRUE;

    } else {

        //
        //  This is not the fist usage, so write back to user.
        //

        *FirstUsage = FALSE;

        //
        //  This is not the first query. Lets make sure that our data 
        //  is consistent. If the information classes don't line up
        //  then our cache might be inconsistant. We should fail this
        //  operation.
        //
        //  TODO: Is this correct?
        //

        if (DirContext->InformationClass != InformationClass) {

            Status = STATUS_INVALID_PARAMETER;
            goto NcStreamHandleContextEnumSetupCleanup;
        }
    }

    if (!DirContext->InUse || ResetSearch) {

        //
        //  Either this is the first use of the context,
        //  or they are reseting the enumeration. We 
        //  should clear the cache either way.
        //

        if (DirContext->Cache.Buffer != NULL) {
            
            ExFreePoolWithTag( DirContext->Cache.Buffer, NC_TAG );
            DirContext->Cache.Buffer = NULL;
            DirContext->Cache.CurrentOffset = 0;
        }

        if (DirContext->InjectionEntry.Buffer != NULL) {

            ExFreePoolWithTag( DirContext->InjectionEntry.Buffer, NC_TAG );
            DirContext->InjectionEntry.Buffer = NULL;
            DirContext->InjectionEntry.CurrentOffset = 0;
        }

        //
        //  Now that the cache is clear we can set up the injection entry.
        //  The injection entry is the user mapping itself. Thus it only needs
        //  to be injected if the directory being enumerated is the parent of
        //  the user mapping.
        //

        if (UserMappingOverlap.Parent) {

            Status = NcEnumerateDirectorySetupInjection( DirContext,
                                                         FltObjects,
                                                         InstanceContext,
                                                         Offsets,
                                                         InformationClass ); 
            if (!NT_SUCCESS( Status )) {
                
                goto NcStreamHandleContextEnumSetupCleanup;
            }
        }
    }

    // 
    //  Now we know that the entry is setup.
    //  Mark it as in use.
    //
    
    DirContext->InUse = TRUE;
    Status = STATUS_SUCCESS;

NcStreamHandleContextEnumSetupCleanup:

    if (!NT_SUCCESS( Status )) {

        if (!DirContext->InUse) {

            //
            //  We failed to set up the context for first use.
            //  We should free our buffer we allocated.
            //

            if (DirContext->SearchString.Buffer != NULL) {

                ExFreePoolWithTag( DirContext->SearchString.Buffer, NC_TAG );
                DirContext->SearchString.Buffer = NULL;
                DirContext->SearchString.Length = 0;
                DirContext->SearchString.MaximumLength = 0;
            }
        }
    }

    return Status;
}

VOID 
NcStreamHandleContextEnumClose (
    _In_ PNC_DIR_QRY_CONTEXT DirContext 
    )
/*++

Routine Description:

    Tears down any remaining state associated with a directory enumeration.

Arguments:

    DirContext - Pointer to the directory context.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    if (DirContext->Cache.Buffer != NULL) {

        ExFreePoolWithTag( DirContext->Cache.Buffer,
                           NC_DIR_QRY_CACHE_TAG );

        DirContext->Cache.Buffer = NULL;
    }

    if (DirContext->InjectionEntry.Buffer != NULL)  {

        ExFreePoolWithTag( DirContext->InjectionEntry.Buffer,
                           NC_DIR_QRY_CACHE_TAG );

        DirContext->InjectionEntry.Buffer = NULL;
    }

    if (DirContext->SearchString.Buffer != NULL) {

        ExFreePoolWithTag( DirContext->SearchString.Buffer,
                           NC_DIR_QRY_SEARCH_STRING );

        DirContext->SearchString.Buffer = NULL;
    }
}


