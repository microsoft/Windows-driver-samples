/*++

Copyright (c) 2008 - 2009  Microsoft Corporation

Module Name:

    ncfileinfo.c

Abstract:

    Contains routines to process user-initiated query file and set file
    information requests.  

Environment:

    Kernel mode

--*/

#include "nc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcPreQueryAlternateName)
#pragma alloc_text(PAGE, NcPostQueryHardLinks)
#pragma alloc_text(PAGE, NcPostQueryName)
#pragma alloc_text(PAGE, NcPreSetDisposition)
#pragma alloc_text(PAGE, NcPreSetLinkInformation)
#pragma alloc_text(PAGE, NcPreSetShortName)
#pragma alloc_text(PAGE, NcPreRename)
#endif

FLT_POSTOP_CALLBACK_STATUS
NcPostQueryName (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called when the user wants to request a name for a
    previously opened handle.  Since we munged the name to be to the real
    mapping in pre-create, we must munge it back to the user visible view
    in response to name requests, even by opened name.

    Note that this function processes three information classes:

    FileNameInformation
    FileNormalizedNameInformation
    FileAllInformation

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
    NTSTATUS Status = STATUS_SUCCESS;
    NC_PATH_OVERLAP Overlap;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    FILE_INFORMATION_CLASS InfoClass = Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;
    PVOID UserBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG UserBufferLength = Data->Iopb->Parameters.QueryFileInformation.Length;
    ULONG SizeActuallyReturned = (ULONG)Data->IoStatus.Information;
    ULONG LengthNeeded = 0;
    ULONG UserStructureSize = 0;
    ULONG RequiredNameSize = 0;
    ULONG NameLengthAvailable = 0;
    PFILE_NAME_INFORMATION NameInfo;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );
    UNICODE_STRING Remainder;
    UNICODE_STRING RemainderCopy = EMPTY_UNICODE_STRING;
    UNICODE_STRING ReturnedName;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  If the operation failed already, we have no processing to do unless the
    //  failure is a buffer overflow.
    //

    if (!NT_SUCCESS( Data->IoStatus.Status ) &&
        (Data->IoStatus.Status != STATUS_BUFFER_OVERFLOW)) {

        Status = Data->IoStatus.Status;
        goto NcPostQueryNameInformationCleanup;
    }

    //
    //  Get our instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        goto NcPostQueryNameInformationCleanup;
    }

    //
    //  Find the name in the user buffer.
    //
    
    if (InfoClass == FileAllInformation) {

        NameInfo = & ((PFILE_ALL_INFORMATION) UserBuffer)->NameInformation;

    } else {

        FLT_ASSERT( InfoClass == FileNameInformation ||
                    InfoClass == FileNormalizedNameInformation );

        NameInfo = UserBuffer;
    }

    //
    //  If the name is too long for a UNICODE_STRING, we can't process it.
    //  This should never really happen, since UNICODE_STRINGs are used
    //  all across the NT IO model.
    //

    if (NameInfo->FileNameLength >= MAXUSHORT) {

        Status = STATUS_OBJECT_PATH_INVALID;
        goto NcPostQueryNameInformationCleanup;
    }

    //
    //  Now that we have an instance context and NameInfo buffer, see if the file
    //  system failed with a buffer overflow.
    //

    if (Data->IoStatus.Status == STATUS_BUFFER_OVERFLOW) {

        //
        //  We need to bias the FileNameLength field by the difference between
        //  the real and user mapping lengths if the user mapping is longer.  This
        //  is so that if the caller re-issues the name query we won't fail with
        //  a buffer overflow in the filter even if the file system succeeded.
        //

        if (InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Length > 
            InstanceContext->Mapping.RealMapping.LongNamePath.VolumelessName.Length) {

            NameInfo->FileNameLength += InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Length -
                                        InstanceContext->Mapping.RealMapping.LongNamePath.VolumelessName.Length;
        }

        Status = Data->IoStatus.Status;
        LengthNeeded = SizeActuallyReturned;

        goto NcPostQueryNameInformationCleanup;
    }

    ReturnedName.Buffer = NameInfo->FileName;
    ReturnedName.MaximumLength = 
    ReturnedName.Length = (USHORT)NameInfo->FileNameLength;
    
    //
    //  Check if the name being returned is within the real mapping.
    //  If not, we have no translation to perform.
    //

    NcComparePath( &ReturnedName,
                   &InstanceContext->Mapping.RealMapping,
                   &Remainder,
                   IgnoreCase,
                   FALSE,
                   &Overlap );

    if (!Overlap.InMapping && !Overlap.Match) {

        Status = Data->IoStatus.Status;
        LengthNeeded = SizeActuallyReturned;

        goto NcPostQueryNameInformationCleanup;
    }

    //
    //  Make sure that the user buffer is long enough.
    //

    UserStructureSize = FIELD_OFFSET( FILE_NAME_INFORMATION, FileName );

    if (InfoClass == FileAllInformation) {

        UserStructureSize += FIELD_OFFSET( FILE_ALL_INFORMATION, NameInformation );
    }

    RequiredNameSize = InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Length;

    //
    //  Add back the trailing portion of the name.  Note that Remainder
    //  is only defined if InMapping is TRUE.
    //

    if (Overlap.InMapping && !Overlap.Match) {

        RequiredNameSize += Remainder.Length + sizeof(WCHAR);
    }

    LengthNeeded = UserStructureSize + RequiredNameSize;

    //
    //  Whether the user has provided enough buffer or not, the
    //  FILE_NAME_INFORMATION.FileNameLength field has to contain the total length
    //  of the name we want to return.
    //

    NameInfo->FileNameLength = RequiredNameSize;

    //
    //  If the user's buffer is not big enough to handle the name we need to return,
    //  we will copy in as much as we can and return STATUS_BUFFER_OVERFLOW.  The
    //  user expects that the needed name length will be reported in the
    //  FILE_NAME_INFORMATION.FileNameLength field.
    //

    if (UserBufferLength < LengthNeeded) {

        NameLengthAvailable = UserBufferLength - UserStructureSize;

        //
        //  Truncate the LengthNeeded value since it will be returned in the
        //  IoStatus block to tell I/O Manager how much to copy back to the
        //  user's buffer.
        //

        LengthNeeded = UserBufferLength;

        Status = STATUS_BUFFER_OVERFLOW;

    //
    //  We have enough space.  Let's assume we'll succeed to copy the name.
    //

    } else {
        
        NameLengthAvailable = NameInfo->FileNameLength;

        Status = STATUS_SUCCESS;
    }

    if (Overlap.InMapping && !Overlap.Match) {

        //
        //  Copy the remainder of the returned name from the user.  This is
        //  done so that we can rewrite the user's buffer.  Note that if we
        //  are not a match, we expect some remainder.
        //

        FLT_ASSERT( Remainder.Length > 0 );

        RemainderCopy.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                      Remainder.Length,
                                                      NC_TAG );

        if (RemainderCopy.Buffer == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto NcPostQueryNameInformationCleanup;
        }

        RtlCopyMemory( RemainderCopy.Buffer,
                       Remainder.Buffer,
                       Remainder.Length );

        RemainderCopy.MaximumLength =
        RemainderCopy.Length = Remainder.Length;
    }

    //
    //  Firstly, copy back the name to our mapping.
    //

    RtlCopyMemory( &NameInfo->FileName, 
                   InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Buffer, 
                   min(InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Length,
                       NameLengthAvailable) );

    if (NameLengthAvailable > InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Length) {

        NameLengthAvailable -= InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Length;

    } else {
        
        NameLengthAvailable = 0;
    }

    //
    //  If the object being queried is within the mapping, copy back the
    //  remainder of that name.
    //

    if ((NameLengthAvailable > 0) &&
        Overlap.InMapping && !Overlap.Match) {

        NameInfo->FileName[InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Length / sizeof(WCHAR)] = '\\';

        RtlCopyMemory( Add2Ptr( &NameInfo->FileName,
                                InstanceContext->Mapping.UserMapping.LongNamePath.VolumelessName.Length + sizeof(WCHAR) ),
                       RemainderCopy.Buffer,
                       min(RemainderCopy.Length, NameLengthAvailable) );
    }

    //
    //  We have finished the query, complete operation.
    //
    
NcPostQueryNameInformationCleanup:

    Data->IoStatus.Status = Status;

    //
    //  Note that STATUS_BUFFER_OVERFLOW is not a success code, but for name queries
    //  it indicates that the caller needs to allocate a bigger buffer.  The needed
    //  size for the name is stored in the FILE_NAME_INFORMATION.FileNameLength field.
    //  Therefore the IoStatus.Information field must not be 0 for a buffer overflow,
    //  it must contain the size of the data that was actually copied.
    //

    if (NT_SUCCESS( Status ) ||
        (Status == STATUS_BUFFER_OVERFLOW)) {

        Data->IoStatus.Information = LengthNeeded;

    } else {

        Data->IoStatus.Information = 0;
    }

    if (RemainderCopy.Buffer != NULL) {

        ExFreePoolWithTag( RemainderCopy.Buffer, NC_TAG );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
NcPreQueryAlternateName (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is called when the user wants to find the alternate name
    for a previously opened handle.  An alternate name means the short
    half of a long/short name pair.

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
    NTSTATUS Status;
    FLT_PREOP_CALLBACK_STATUS ReturnValue;
    NC_PATH_OVERLAP Overlap;
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    PVOID UserBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG UserBufferLength = Data->Iopb->Parameters.QueryFileInformation.Length;
    ULONG LengthNeeded = 0;
    PFILE_NAME_INFORMATION NameInfo;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );
    PUNICODE_STRING FinalComponentToReturn;

    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  Get our instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreQueryAlternateNameInformationCleanup;
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
        goto NcPreQueryAlternateNameInformationCleanup;
    }

    Status = FltParseFileNameInformation( FileInfo );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreQueryAlternateNameInformationCleanup;
    }

    //
    //  We only need to handle the case where a shortname is being
    //  generated on the mapping itself.  These names are final
    //  component path only, so any files within the mapping will
    //  still be correct even if we don't munge them.
    //

    NcComparePath( &FileInfo->Name,
                   &InstanceContext->Mapping.UserMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &Overlap );

    if (!Overlap.Match) {

        NcComparePath( &FileInfo->Name,
                       &InstanceContext->Mapping.RealMapping,
                       NULL,
                       IgnoreCase,
                       TRUE,
                       &Overlap );

        FLT_ASSERT( !Overlap.Match );

        if (!Overlap.Match) {

            //
            //  This file is not the mapping, so we can just let this
            //  request go down normally.
            //

            ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
            goto NcPreQueryAlternateNameInformationCleanup;
        }
    }

    //
    //  Return the short name.
    //
    //  Note that this behavior differs from the filesystem in two respects:
    //
    //  1. We are not attempting to detect (and fail for) an open-by-ID
    //     handle.  Since these are link agnostic, returning data is
    //     meaningless.
    //
    //  2. We may support having multiple alternate names for multiple
    //     links on the file, if the mapping was created as a file then
    //     a hardlink was created with a mapping name.  This file thus
    //     contains two shortnames, which NTFS does not support.  In
    //     theory APIs should be clean to this (and a future filesystem
    //     may wish to support it.)
    //

    FinalComponentToReturn = &InstanceContext->Mapping.UserMapping.ShortNamePath.FinalComponentName;

    //
    //  Make sure that the user buffer is long enough.
    //

    LengthNeeded = FIELD_OFFSET( FILE_NAME_INFORMATION, FileName );

    LengthNeeded += FinalComponentToReturn->Length;

    if (UserBufferLength < LengthNeeded) {

        Status = STATUS_BUFFER_OVERFLOW;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreQueryAlternateNameInformationCleanup;
    }

    NameInfo = UserBuffer;

    //
    //  Copy back the name to our mapping.
    //

    RtlCopyMemory( &NameInfo->FileName, 
                   FinalComponentToReturn->Buffer, 
                   FinalComponentToReturn->Length );

    NameInfo->FileNameLength = FinalComponentToReturn->Length;

    //
    //  We have finished the query, complete operation.
    //
    
    Status = STATUS_SUCCESS;
    ReturnValue = FLT_PREOP_COMPLETE;

NcPreQueryAlternateNameInformationCleanup:

    if (ReturnValue == FLT_PREOP_COMPLETE) {

        Data->IoStatus.Status = Status;

        //
        //  Note that STATUS_BUFFER_OVERFLOW is not a success code, and
        //  will result in zero bytes being copied back to the caller.
        //

        if (NT_SUCCESS( Status )) {

            Data->IoStatus.Information = LengthNeeded;
        } else {

            Data->IoStatus.Information = 0;
        }
    }

    if (FileInfo != NULL) {

        FltReleaseFileNameInformation( FileInfo );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    return ReturnValue;
}


FLT_POSTOP_CALLBACK_STATUS
NcPostQueryHardLinks (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called when the user wants to enumerate all hard links
    for a previously opened handle.

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
    NTSTATUS Status;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    //
    //  Pointer to the buffer returned to us, which we will also return to
    //  our caller; length of the buffer; size of the buffer filled in
    //  by the filesystem; size of the buffer that we filled in
    //

    PFILE_LINKS_INFORMATION UserBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
    ULONG UserBufferLength = Data->Iopb->Parameters.QueryFileInformation.Length;
    ULONG SizeActuallyReturned = (ULONG)Data->IoStatus.Information;
    ULONG BytesWritten = 0;

    //
    //  Length of the current entry that we're processing; a UNICODE_STRING
    //  for the final component of the name that we're processing; and
    //  a flag indicating whether this entry is being modified
    //

    ULONG EntrySize;
    UNICODE_STRING EntryName;
    BOOLEAN MungeEntry;

    //
    //  Copy of the buffer returned from the filesystem, our iterators as
    //  we process this buffer, and a pointer to the previous destination
    //  entry (if one exists) so we can zero the offset to next entry
    //  field on completion
    //

    PFILE_LINKS_INFORMATION OriginalBuffer = NULL;
    PFILE_LINK_ENTRY_INFORMATION SourceEntry;
    PFILE_LINK_ENTRY_INFORMATION DestEntry;
    PFILE_LINK_ENTRY_INFORMATION PrevDestEntry = NULL;


    //
    //  Variables that we use to obtain IDs to the mapping parents.
    //

    OBJECT_ATTRIBUTES MappingParentAttributes;
    HANDLE MappingParentHandle = NULL;
    PFILE_OBJECT MappingParentFileObject = NULL;
    IO_STATUS_BLOCK MappingParentStatusBlock;

    //
    //  File IDs for the mapping parents
    //

    LONGLONG RealMappingParentId;
    LONGLONG UserMappingParentId;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  If the buffer was invalid, or if the call failed, leave now.  Note
    //  that this may be STATUS_BUFFER_OVERFLOW and the number of bytes the
    //  caller needs will be reported inaccurately.  To handle this we'd
    //  really need to issue our own call to get the full buffer, then
    //  transform it to find the "correct" length the user will need.
    //
    //  Rather than do this, we return the caller a value for bytes required
    //  which may not be accurate.  When they call us again, we will
    //  have data to transform, and can then fail the call again specifying
    //  a new value for bytes required that is accurate.
    //

    if (SizeActuallyReturned <= (ULONG)FIELD_OFFSET( FILE_LINKS_INFORMATION, Entry ) ||
        !NT_SUCCESS( Data->IoStatus.Status )) {
            
        BytesWritten = SizeActuallyReturned;

        Status = Data->IoStatus.Status;
        goto NcPostQueryHardLinkInformationCleanup;
    }

    //
    //  Get our instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        goto NcPostQueryHardLinkInformationCleanup;
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

        goto NcPostQueryHardLinkInformationCleanup;
    }

    Status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                      MappingParentFileObject,
                                      &RealMappingParentId,
                                      sizeof(RealMappingParentId),
                                      FileInternalInformation,
                                      NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcPostQueryHardLinkInformationCleanup;
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

        goto NcPostQueryHardLinkInformationCleanup;
    }

    Status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                      MappingParentFileObject,
                                      &UserMappingParentId,
                                      sizeof(UserMappingParentId),
                                      FileInternalInformation,
                                      NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcPostQueryHardLinkInformationCleanup;
    }

    FltClose( MappingParentHandle );
    ObDereferenceObject( MappingParentFileObject );

    MappingParentHandle = NULL;
    MappingParentFileObject = NULL;

    //
    //  Take a copy of the results of the call from the filesystem.
    //

    OriginalBuffer = ExAllocatePoolWithTag( PagedPool,
                                            SizeActuallyReturned,
                                            NC_TAG );

    if (OriginalBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcPostQueryHardLinkInformationCleanup;
    }

    RtlCopyMemory( OriginalBuffer, UserBuffer, SizeActuallyReturned );

    //
    //  Set up our iterators to walk through the returned links.
    //

    DestEntry = &UserBuffer->Entry;

    if (UserBuffer->EntriesReturned >= 1) {
        SourceEntry = &OriginalBuffer->Entry;
    } else {
        SourceEntry = NULL;
    }
    UserBuffer->EntriesReturned = 0;
    BytesWritten = FIELD_OFFSET( FILE_LINKS_INFORMATION, Entry );
    UserBuffer->BytesNeeded = BytesWritten;

    while( SourceEntry ) {

        //
        //  Assume we don't need to munge the link.  If the parent IDs,
        //  final component lengths and final component strings correspond
        //  to the real mapping, we will need to transform it.
        //

        MungeEntry = FALSE;

        if (SourceEntry->ParentFileId == RealMappingParentId) {

            if (SourceEntry->FileNameLength * sizeof(WCHAR) >= MAXUSHORT) {

                Status = STATUS_OBJECT_PATH_INVALID;
                goto NcPostQueryHardLinkInformationCleanup;
            }

            EntryName.Buffer = SourceEntry->FileName;
            EntryName.Length = EntryName.MaximumLength = (USHORT)SourceEntry->FileNameLength * sizeof(WCHAR);

            if (EntryName.Length == InstanceContext->Mapping.RealMapping.LongNamePath.FinalComponentName.Length &&
                RtlCompareUnicodeString( &EntryName, &InstanceContext->Mapping.RealMapping.LongNamePath.FinalComponentName, IgnoreCase ) == 0) {

                MungeEntry = TRUE;
            }

            //
            //  TODO: Preserve shortness in output
            //

            if (EntryName.Length == InstanceContext->Mapping.RealMapping.ShortNamePath.FinalComponentName.Length &&
                RtlCompareUnicodeString( &EntryName, &InstanceContext->Mapping.RealMapping.ShortNamePath.FinalComponentName, IgnoreCase ) == 0) {

                MungeEntry = TRUE;
            }
        }

        //
        //  Calculate the length of the entry that we want to write.
        //

        if (MungeEntry) {

            EntrySize = FIELD_OFFSET( FILE_LINK_ENTRY_INFORMATION, FileName ) +
                        InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Length;

            EntrySize = AlignToSize( EntrySize, 8 );

        } else {

            EntrySize = FIELD_OFFSET( FILE_LINK_ENTRY_INFORMATION, FileName ) +
                        SourceEntry->FileNameLength * sizeof(WCHAR);

            EntrySize = AlignToSize( EntrySize, 8 );
        }

        //
        //  Record how much space the caller would need to return all entries.
        //

        UserBuffer->BytesNeeded += EntrySize;

        //
        //  If we have space, copy this entry into the user's buffer and
        //  advance our destination iterator.
        //

        if (BytesWritten + EntrySize <= UserBufferLength) {

            if (MungeEntry) {

                DestEntry->NextEntryOffset = EntrySize;
                DestEntry->ParentFileId = UserMappingParentId;
                DestEntry->FileNameLength = InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Length / sizeof(WCHAR);
                RtlCopyMemory( DestEntry->FileName,
                               InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Buffer,
                               InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Length );
            } else {

                RtlCopyMemory( DestEntry, SourceEntry, EntrySize );
            }
            PrevDestEntry = DestEntry;
            DestEntry = Add2Ptr( DestEntry, EntrySize );
            UserBuffer->EntriesReturned++;
            BytesWritten += EntrySize;
        }

        //
        //  If we still have links that we have not yet consumed, move
        //  to those.
        //

        if (SourceEntry->NextEntryOffset != 0) {
            SourceEntry = Add2Ptr( SourceEntry, SourceEntry->NextEntryOffset );
        } else {
            SourceEntry = NULL;
        }
    }

    //
    //  If we already copied one or more links, make sure our list is
    //  correctly terminated.
    //

    if (PrevDestEntry != NULL) {
        PrevDestEntry->NextEntryOffset = 0;
    }

    //
    //  If we copied all results, return STATUS_SUCCESS.  If we saw entries
    //  that we did not copy, return STATUS_BUFFER_OVERFLOW.
    //

    if (BytesWritten == UserBuffer->BytesNeeded) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_BUFFER_OVERFLOW;
    }

NcPostQueryHardLinkInformationCleanup:

    Data->IoStatus.Status = Status;

    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {

        Data->IoStatus.Information = BytesWritten;
    } else {

        Data->IoStatus.Information = 0;
    }

    if (MappingParentHandle != NULL) {

        FltClose( MappingParentHandle );
        MappingParentHandle = NULL;
    }

    if (MappingParentFileObject != NULL) {

        ObDereferenceObject( MappingParentFileObject );
        MappingParentFileObject = NULL;
    }

    if (OriginalBuffer != NULL) {

        ExFreePoolWithTag( OriginalBuffer, NC_TAG );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
NcPreSetShortName (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is called when the user wants to change the short name
    for a previously opened handle.

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
    PFILE_NAME_INFORMATION NameInfo = 
        Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NC_PATH_OVERLAP RealOverlap;
    NC_PATH_OVERLAP UserOverlap;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    UNREFERENCED_PARAMETER( CompletionContext );

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetShortNameCleanup;
    }

    //
    //  Let's skip doing any of this work for file systems that we know don't
    //  have short names.
    //

    if ((InstanceContext->VolumeFilesystemType == FLT_FSTYPE_EXFAT) ||
        (InstanceContext->VolumeFilesystemType == FLT_FSTYPE_REFS)) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetShortNameCleanup;
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
        goto NcPreSetShortNameCleanup;
    }

    Status = FltParseFileNameInformation( FileInfo );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetShortNameCleanup;
    }

    //
    //  Calculate Overlap and Remainder
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

    //
    //  Currently the file names that we use are read only, so changing a
    //  shortname on the mapping or any of its ancestors cannot be
    //  supported.
    //
    //  TODO: Should we support this?
    //

    if (RealOverlap.Match ||
        UserOverlap.Match ||
        RealOverlap.Ancestor ||
        UserOverlap.Ancestor ||
        NameInfo->FileNameLength > MAXUSHORT) {

        Status = STATUS_ACCESS_DENIED;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetShortNameCleanup;
    }

    //
    //  If the user is attempting to set a name which is used by the real
    //  file, we let the request go to the file system (which will fail
    //  it.)  For names used by the user mapping, we need to detect and
    //  fail those.
    //

    if (UserOverlap.Peer) {

        UNICODE_STRING TargetComponent;

        TargetComponent.Buffer = NameInfo->FileName;
        TargetComponent.Length = TargetComponent.MaximumLength = (USHORT)NameInfo->FileNameLength;

        if( RtlCompareUnicodeString( &TargetComponent,
                                     &InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName,
                                     IgnoreCase) == 0 ||
            RtlCompareUnicodeString( &TargetComponent,
                                     &InstanceContext->Mapping.UserMapping.ShortNamePath.FinalComponentName,
                                     IgnoreCase) == 0 ) {

            Status = STATUS_ACCESS_DENIED;
            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcPreSetShortNameCleanup;
        }

    }

    //
    //  If the user is not setting a short name on our mapping, an ancestor
    //  of our mapping or targetting our mapping, let the request go to the
    //  file system.
    //

    Status = STATUS_SUCCESS;
    ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

NcPreSetShortNameCleanup:

    if (ReturnValue == FLT_PREOP_COMPLETE) {

        Data->IoStatus.Status = Status;
    }

    if (FileInfo != NULL) {

        FltReleaseFileNameInformation( FileInfo );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    return ReturnValue;
}


FLT_PREOP_CALLBACK_STATUS
NcPreSetDisposition (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Fltmgr callback which manages setting the delete disposition on a file.
    We must disallow setting the delete disposition on an ancestor of either 
    mapping because otherwise we would have to maintain the mapping's
    short/long name pairings.

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
    NTSTATUS Status;
    FLT_PREOP_CALLBACK_STATUS ReturnValue;
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    PNC_MAPPING Mapping;
    NC_PATH_OVERLAP RealOverlap;
    NC_PATH_OVERLAP UserOverlap;
    PFILE_DISPOSITION_INFORMATION Disposition;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    PAGED_CODE();

    UNREFERENCED_PARAMETER( CompletionContext );

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  See if they are setting the delete disposition to false.
    //  If they are we can passthrough. We don't care if people
    //  want to mark the mapping as "don't delete".
    //

    Disposition = Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
    if (Disposition->DeleteFile == FALSE) {

        Status = STATUS_SUCCESS;
        goto NcPreSetDispositionCleanup;
    }

    //
    //  The user is trying to delete a file.
    //  We have to make sure that the file is not an ancestor of either mapping.
    //

    //
    //  Get the file's name.
    //

    Status = NcGetFileNameInformation( Data,
                                       NULL,
                                       NULL,
                                       FLT_FILE_NAME_OPENED | 
                                           FLT_FILE_NAME_QUERY_DEFAULT | 
                                           FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                       &FileInfo);

    if (!NT_SUCCESS( Status )) {

        goto NcPreSetDispositionCleanup;
    }

    Status = FltParseFileNameInformation( FileInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcPreSetDispositionCleanup;
    }

    //
    //  Get the mapping
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext );

    if (!NT_SUCCESS( Status )) {
        
        goto NcPreSetDispositionCleanup;
    }

    Mapping = &InstanceContext->Mapping;

    //
    //  Check to see of this will delete an ancestor of the real mapping.
    //

    NcComparePath( &FileInfo->Name,
                   &Mapping->RealMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &RealOverlap );

    if (RealOverlap.Ancestor) {

        //
        //  The file is an ancestor of the real mapping, so disallow setting
        //  disposition.
        //

        Status = STATUS_ACCESS_DENIED;
        goto NcPreSetDispositionCleanup;
    }

    //
    //  Check the user mapping.
    //

    NcComparePath( &FileInfo->Name,
                   &Mapping->UserMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &UserOverlap );

    if (UserOverlap.Ancestor) {

        //
        //  The file is an ancestor of the user mapping, so disallow setting
        //  disposition.
        //

        Status = STATUS_ACCESS_DENIED;
        goto NcPreSetDispositionCleanup;
    }

    //
    //  The file is ok to mark for delete.
    //

    Status = STATUS_SUCCESS;
    goto NcPreSetDispositionCleanup;

NcPreSetDispositionCleanup:

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        Data->IoStatus.Status = Status;

    } else {

        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (FileInfo) {

        FltReleaseFileNameInformation( FileInfo );
    }

    if (InstanceContext) {

        FltReleaseContext( InstanceContext );
    }

    return ReturnValue;
}


FLT_PREOP_CALLBACK_STATUS
NcPreSetLinkInformation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Fltmgr callback which manages link creation on a file.
    We need to make sure that new links down the user mapping
    are redirected to the real mapping.

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
    FLT_PREOP_CALLBACK_STATUS ReturnValue;
    NTSTATUS Status;
    PFILE_LINK_INFORMATION LinkInfo = 
        Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
    PFILE_LINK_INFORMATION MungedLinkInfo = NULL;
    ULONG MungedLinkInfoSize;
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NC_PATH_OVERLAP RealOverlap;
    NC_PATH_OVERLAP UserOverlap;
    UNICODE_STRING UserRemainder;
    UNICODE_STRING MungedName = EMPTY_UNICODE_STRING;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    UNREFERENCED_PARAMETER( CompletionContext );

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext);

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetLinkInformationCleanup;
    }

    Status = FltGetDestinationFileNameInformation( FltObjects->Instance,
                                                   FltObjects->FileObject,
                                                   LinkInfo->RootDirectory,
                                                   LinkInfo->FileName,
                                                   LinkInfo->FileNameLength,
                                                   FLT_FILE_NAME_OPENED |
                                                       FLT_FILE_NAME_QUERY_DEFAULT |
                                                       FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                                   &FileInfo);

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetLinkInformationCleanup;
    }

    Status = FltParseFileNameInformation( FileInfo );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetLinkInformationCleanup;
    }

    //
    //  Calculate Overlap and Remainder
    //

    NcComparePath( &FileInfo->Name,
                   &InstanceContext->Mapping.UserMapping,
                   &UserRemainder,
                   IgnoreCase,
                   TRUE,
                   &UserOverlap );

    NcComparePath( &FileInfo->Name,
                   &InstanceContext->Mapping.RealMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &RealOverlap );

    //
    //  We cannot allow the user to link inside the real mapping, since it
    //  is hidden.
    //

    if (RealOverlap.Match) {

        Status = STATUS_ACCESS_DENIED;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetLinkInformationCleanup;

    } else if (RealOverlap.InMapping) {

        //
        //  We should never get here.  Getting here requires an
        //  OPEN_TARGET_DIRECTORY open which should already have failed.
        //

        FLT_ASSERT( !RealOverlap.InMapping );

        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetLinkInformationCleanup;

    } else if ((RealOverlap.Ancestor || UserOverlap.Ancestor) &&
               LinkInfo->ReplaceIfExists) {

        //
        //  The user is attempting to overwrite a parent of the mapping.
        //  Fail this operation.
        //

        Status = STATUS_ACCESS_DENIED;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetLinkInformationCleanup;
    }

    //
    //  If the destination path is outside the mapping then we can pass it
    //  through without a problem.
    //

    if (!UserOverlap.InMapping) {

        //
        //  The destination outside the mapping.  We can ignore this IO.
        //

        Status = STATUS_SUCCESS;
        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreSetLinkInformationCleanup;
    }

    //
    //  The destination is inside the mapping.  This means we have to issue
    //  our own request and forward the results to the user.
    //

    //
    //  We need to build a new path to link on.
    //
    
    Status = NcConstructPath( &InstanceContext->Mapping.RealMapping,
                              &UserRemainder,
                              TRUE,
                              &MungedName );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetLinkInformationCleanup;
    }

    //
    //  Create our own link structure.
    //  
    
    MungedLinkInfoSize = sizeof(FILE_LINK_INFORMATION) + MungedName.Length - sizeof(WCHAR);
    MungedLinkInfo = ExAllocatePoolWithTag( PagedPool,
                                            MungedLinkInfoSize,
                                            NC_SET_LINK_BUFFER_TAG );

    if (MungedLinkInfo == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreSetLinkInformationCleanup;
    }

    MungedLinkInfo->ReplaceIfExists = LinkInfo->ReplaceIfExists;
    MungedLinkInfo->RootDirectory = NULL;
    MungedLinkInfo->FileNameLength = MungedName.Length;

    RtlCopyMemory( &MungedLinkInfo->FileName, MungedName.Buffer, MungedName.Length );

    //
    //  Issue our own request.
    //
    
    Status = FltSetInformationFile( FltObjects->Instance,
                                    FltObjects->FileObject,
                                    MungedLinkInfo,
                                    MungedLinkInfoSize,
                                    FileLinkInformation );

    //
    //  Because we issued the IO, we will pass complete this ourselves.
    //
    
    ReturnValue = FLT_PREOP_COMPLETE;

NcPreSetLinkInformationCleanup:

    if (ReturnValue == FLT_PREOP_COMPLETE) {

        Data->IoStatus.Status = Status;
    }

    if (MungedLinkInfo != NULL) {

        ExFreePoolWithTag( MungedLinkInfo, NC_SET_LINK_BUFFER_TAG );
    }

    if (FileInfo != NULL) {

        FltReleaseFileNameInformation( FileInfo );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (MungedName.Buffer != NULL) {

        ExFreePoolWithTag( MungedName.Buffer, NC_TAG );
    }

    return ReturnValue;
}

FLT_PREOP_CALLBACK_STATUS
NcPreRename(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Fltmgr callback which manages renaming files.  We must disallow renaming
    on an ancestor of either mapping because otherwise we would have to
    maintain the mapping's short/long name pairings.

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
    //
    //  Return Values
    //
    
    NTSTATUS Status;
    FLT_PREOP_CALLBACK_STATUS ReturnValue;

    //
    //  Contexts
    //
    
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
   
    //
    //  Data
    //
    
    PFILE_RENAME_INFORMATION RenameInfo = 
        Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
    
    //
    //  FileInformation
    //
    
    PFLT_FILE_NAME_INFORMATION TargetInfo = NULL;
    PFLT_FILE_NAME_INFORMATION SrcInfo = NULL;
    
    //
    //  Target Real Overlap
    //
    
    NC_PATH_OVERLAP TargetRealOverlap;
    UNICODE_STRING TargetRealRemainder;
    
    //
    //  Target User Overlap
    //
    
    NC_PATH_OVERLAP TargetUserOverlap;
    UNICODE_STRING TargetUserRemainder;

    //
    //  Src Real Overlap
    //
    
    NC_PATH_OVERLAP SrcRealOverlap;
    NC_PATH_OVERLAP SrcUserOverlap;
    
    //
    //  Munge Data
    //
    
    UNICODE_STRING MungedTargetName = EMPTY_UNICODE_STRING;
    PFILE_RENAME_INFORMATION MungedRenameInfo = NULL;
    ULONG MungedRenameLength;
    BOOLEAN IgnoreCase = !BooleanFlagOn( FltObjects->FileObject->Flags,
                                         FO_OPENED_CASE_SENSITIVE );

    PAGED_CODE();

    UNREFERENCED_PARAMETER( CompletionContext );

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    
    //
    //  Get Instance Context 
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreRenameCleanup;
    }

    //
    //  Find out the src file's name.
    //
    
    Status = NcGetFileNameInformation( Data,
                                       NULL,
                                       NULL,
                                       FLT_FILE_NAME_OPENED | 
                                           FLT_FILE_NAME_QUERY_DEFAULT | 
                                           FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                       &SrcInfo);

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreRenameCleanup;
    }

    Status = FltParseFileNameInformation( SrcInfo );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreRenameCleanup;
    }

    //
    //  Find the src's overlap with the real and user mappings.
    //
    
    NcComparePath( &SrcInfo->Name,
                   &InstanceContext->Mapping.RealMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &SrcRealOverlap );

    NcComparePath( &SrcInfo->Name,
                   &InstanceContext->Mapping.UserMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &SrcUserOverlap );

    //
    //  If the src is an ancestor of either the user or real mappings we can
    //  fail the request.
    //
    
    if (SrcUserOverlap.Ancestor || SrcRealOverlap.Ancestor) {

        ReturnValue = FLT_PREOP_COMPLETE;
        Status = STATUS_ACCESS_DENIED;
        goto NcPreRenameCleanup;
    }

    //
    //  Find out the target file's name.
    //
    
    Status = FltGetDestinationFileNameInformation( FltObjects->Instance,
                                                   FltObjects->FileObject,
                                                   RenameInfo->RootDirectory,
                                                   RenameInfo->FileName,
                                                   RenameInfo->FileNameLength,
                                                   FLT_FILE_NAME_OPENED |
                                                       FLT_FILE_NAME_QUERY_DEFAULT |
                                                       FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER,
                                                   &TargetInfo);

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreRenameCleanup;
    }

    Status = FltParseFileNameInformation( TargetInfo );

    if( !NT_SUCCESS( Status ) ) {
        
        FLT_ASSERT( NT_SUCCESS( Status ) );

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreRenameCleanup;
    }

    //
    //  Find the target's overlap with the real and user mappings.
    //
    
    NcComparePath( &TargetInfo->Name,
                   &InstanceContext->Mapping.RealMapping,
                   &TargetRealRemainder,
                   IgnoreCase,
                   TRUE,
                   &TargetRealOverlap );

    NcComparePath( &TargetInfo->Name,
                   &InstanceContext->Mapping.UserMapping,
                   &TargetUserRemainder,
                   IgnoreCase,
                   TRUE,
                   &TargetUserOverlap );

    //
    //  If the target is in the real mapping, then disallow the rename.  If
    //  the target is to an ancestor of the mappings, this could change IDs
    //  and is therefore also disallowed.
    //

    if (TargetRealOverlap.InMapping) {

        Status = STATUS_ACCESS_DENIED;
        ReturnValue = FLT_PREOP_COMPLETE;    
        goto NcPreRenameCleanup;

    } else if ((TargetRealOverlap.Ancestor || TargetUserOverlap.Ancestor) &&
                RenameInfo->ReplaceIfExists) {

        Status = STATUS_ACCESS_DENIED;
        ReturnValue = FLT_PREOP_COMPLETE;    
        goto NcPreRenameCleanup;
    }


    //
    //  If the target is in the user mapping, then we need to munge the
    //  name and send the request down.  If this is a stream rename we
    //  do not perform the mapping since only the stream name is changing.
    //

    if (TargetUserOverlap.InMapping &&
        (RenameInfo->FileName[0] != ':')) {

        Status = NcConstructPath( &InstanceContext->Mapping.RealMapping,
                                  &TargetUserRemainder,
                                  TRUE,
                                  &MungedTargetName );

        if (!NT_SUCCESS( Status )) {

            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcPreRenameCleanup;
        }


        //
        //  Because the target is in the user mapping, we have to issue
        //  our own rename down the real mapping.
        //

        //
        //  Allocate rename information structure.
        //

        MungedRenameLength = sizeof(FILE_RENAME_INFORMATION) -
                             sizeof(WCHAR) +
                             MungedTargetName.Length;

        MungedRenameInfo = ExAllocatePoolWithTag( PagedPool,
                                                  MungedRenameLength,
                                                  NC_RENAME_BUFFER_TAG );

        if (MungedRenameInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcPreRenameCleanup;
        }

        //
        //  Copy user rename parameters.
        //

        MungedRenameInfo->ReplaceIfExists = RenameInfo->ReplaceIfExists;
        MungedRenameInfo->RootDirectory = NULL;
        MungedRenameInfo->FileNameLength = MungedTargetName.Length;
        RtlCopyMemory( &MungedRenameInfo->FileName, 
                       MungedTargetName.Buffer, 
                       MungedTargetName.Length );

        //
        //  Send the request. Note that we cannot just place the new buffer
        //  in the CallbackData; filesystems use the name from a previous
        //  OPEN_TARGET_DIRECTORY open, so changing the buffer here would
        //  result in unexpected (and undefined!) behavior.
        //

        Status = FltSetInformationFile( FltObjects->Instance,
                                        FltObjects->FileObject,
                                        MungedRenameInfo,
                                        MungedRenameLength,
                                        FileRenameInformation );

        //
        //  Complete the IO.
        //

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreRenameCleanup;

    } else {

        //
        //  The target was outside the mapping. The rename does not have 
        //  to be munged. Pass through.
        //

        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreRenameCleanup;

    }

NcPreRenameCleanup:

    if (ReturnValue == FLT_PREOP_COMPLETE) {

        Data->IoStatus.Status = Status;
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (TargetInfo != NULL) {

        FltReleaseFileNameInformation( TargetInfo );
    }

    if (SrcInfo != NULL) {

        FltReleaseFileNameInformation( SrcInfo );
    }

    if (MungedTargetName.Buffer != NULL) {

        ExFreePoolWithTag( MungedTargetName.Buffer, NC_GENERATE_NAME_TAG );
    }

    if (MungedRenameInfo != NULL) {

        ExFreePoolWithTag( MungedRenameInfo, NC_RENAME_BUFFER_TAG );
    }

    return ReturnValue;
}


