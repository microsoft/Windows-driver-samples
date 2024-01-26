
#include "nc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcGenerateFileName)
#pragma alloc_text(PAGE, NcNormalizeNameComponentEx)
#endif

NTSTATUS
NcGenerateFileName (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Out_ PBOOLEAN CacheFileNameInformation,
    _Inout_ PFLT_NAME_CONTROL OutputNameControl
    )
{
    //
    //  Status vars
    //

    NTSTATUS Status;

    //
    //  State lookup vars
    //

    BOOLEAN Opened = (BOOLEAN)(FileObject->FsContext != NULL);                                       // True if file object is opened.
    BOOLEAN ReturnShortName = (BOOLEAN)(FltGetFileNameFormat(NameOptions) == FLT_FILE_NAME_SHORT);   // True if the user is requesting short name
    BOOLEAN ReturnOpenedName = (BOOLEAN)(FltGetFileNameFormat(NameOptions) == FLT_FILE_NAME_OPENED); // True if user is requesting opened name.
    BOOLEAN ReturnNormalizedName = (BOOLEAN)(FltGetFileNameFormat(NameOptions) == FLT_FILE_NAME_NORMALIZED); // True if user is requesting normalized name.
    BOOLEAN IgnoreCase;
    FLT_FILE_NAME_OPTIONS NameQueryMethod = FltGetFileNameQueryMethod( NameOptions );
    FLT_FILE_NAME_OPTIONS NameFlags = FLT_VALID_FILE_NAME_FLAGS & NameOptions;

    //
    //  File name information
    //

    PFLT_FILE_NAME_INFORMATION LowerNameInfo = NULL; // File name as reported by lower name provider. Will always be down real mapping.
    PFLT_FILE_NAME_INFORMATION ShortInfo = NULL;  // We will use ShortInfo to store the short name if needed.

    //
    //  Contexts
    //

    PNC_INSTANCE_CONTEXT InstanceContext = NULL;

    //
    //  Overlap
    //

    NC_PATH_OVERLAP RealOverlap;
    UNICODE_STRING RealRemainder = EMPTY_UNICODE_STRING;

    //
    //  Temp storage
    //

    UNICODE_STRING MungedName = EMPTY_UNICODE_STRING;

    //
    //  Temp pointer
    //

    PUNICODE_STRING Name = NULL; // Pointer to the name we are going to use.

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  This should never happen, but let's be safe.
    //

    if (!ReturnShortName &&
        !ReturnOpenedName &&
        !ReturnNormalizedName) {

        FLT_ASSERT( FALSE );
        Status = STATUS_NOT_SUPPORTED;
        goto NcGenerateFileNameCleanup;
    }

    RealOverlap.EntireFlags = 0;

    //
    //  To prevent infinite recursion, calls to FltGetFileNameInformation
    //  from generate file name callbacks should not target current provider.
    //

    ClearFlag( NameFlags, FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER );

    //
    //  Fetch the instance context.
    //

    Status = FltGetInstanceContext( Instance, &InstanceContext );

    if (!NT_SUCCESS( Status )) {

        goto NcGenerateFileNameCleanup;
    }

    //
    //  We need to know what the name provider under us thinks the file is called.
    //  If the caller wants the normalized name we query that, otherwise we query
    //  the opened name because we have to compare the full path of the file vs.
    //  the real mapping to determine if the file is mapped.
    //

    Status = NcGetFileNameInformation( Data,
                                       FileObject,
                                       Instance,
                                       (ReturnNormalizedName ? FLT_FILE_NAME_NORMALIZED
                                                             : FLT_FILE_NAME_OPENED) |
                                           NameQueryMethod |
                                           NameFlags,
                                       &LowerNameInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcGenerateFileNameCleanup;
    }

    Status = FltParseFileNameInformation( LowerNameInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcGenerateFileNameCleanup;
    }

    //
    //  Issues With Pre-open path:
    //
    //  1) Poison name cache below name provider:
    //    If a filter above a name provider calls FltGetFileNameInformation on a
    //    file object in his precreate callback, fltmgr will call the name
    //    provider's generate name callback before the name provider's pre create
    //    callback is invoked. Name providers by their nature change names in their
    //    pre-create. Because the name provider has not had the opportunity to
    //    modify the name yet, we need to make sure that fltmgr does not cache the name we
    //    return below us, so we set the FLT_FILE_NAME_DO_NOT_CACHE flag.
    //    //TODO: TRY TO GET ACROSS THAT THIS IS A NAME CHANGER PROBLEM, NOT ALL NAME PROVIDERS NEED TO.
    //

    if (!Opened) {

        SetFlag( NameFlags, FLT_FILE_NAME_DO_NOT_CACHE );

        if (Data) {

            //
            //  NT supports case sensitive and non-case sensitive naming in file systems.
            //  This is handled on a per-open basis. Weather an open is case senstive is
            //  determined by the FO_OPENED_CASE_SENSITIVE flag on the file object.
            //  In pre-create the SL_CASE_SENSITIVE flag on the create IRP specifies the mode.
            //
            //  If this is on an unopened FileObject, it had better be pre-create so we know
            //  how to process the operation.  If we are queried on an unopened FileObject
            //  at any other time we have no way to handle the request.
            //

            FLT_ASSERT( Data->Iopb->MajorFunction == IRP_MJ_CREATE ||
                        Data->Iopb->MajorFunction == IRP_MJ_NETWORK_QUERY_OPEN );

            IgnoreCase = !BooleanFlagOn( Data->Iopb->OperationFlags, SL_CASE_SENSITIVE );

        } else {

            //
            //  If people do unsafe queries on preopened IOs, we cannot
            //  determine if the open is case sensitive or not.
            //  So we cannot determine if this open is down the mapping.
            //  fail.
            //

            FLT_ASSERT( FALSE );
            Status = STATUS_INVALID_PARAMETER;
            goto NcGenerateFileNameCleanup;

        }

    } else {

        //
        //  After a file has been opened, the case sensitivity is stored in the file object.
        //

        IgnoreCase = !BooleanFlagOn( FileObject->Flags, FO_OPENED_CASE_SENSITIVE );
    }

    //
    //  Calculate the overlap with the real mapping.
    //

    NcComparePath( &LowerNameInfo->Name,
                   &InstanceContext->Mapping.RealMapping,
                   &RealRemainder,
                   IgnoreCase,
                   TRUE,
                   &RealOverlap );

    //
    //  Whether we munge depends on what name is requested.
    //

    if (ReturnOpenedName ||
        ReturnNormalizedName) {

        if (Opened &&
            RealOverlap.InMapping) {

            //
            //  We munge the opened name if it overlaps with the real mapping.
            //  The returned path will be down the user mapping.
            //

            Status = NcConstructPath( &InstanceContext->Mapping.UserMapping,
                                      &RealRemainder,
                                      TRUE,
                                      &MungedName);

            if (!NT_SUCCESS( Status )) {

                goto NcGenerateFileNameCleanup;
            }

            Name = &MungedName;

        } else {

            //
            //  We return the queried result if the path is not in the
            //  mapping.
            //

            Name = &LowerNameInfo->Name;

        }

    } else if (ReturnShortName) {

        //
        //  Note that unlike opened names, a query for a shortname only returns
        //  the final component, not the full path.
        //

        // TODO: Assert not preopen

        if (RealOverlap.Match) {

            //
            //  The opened path is the mapping path.
            //  This means that if we queried the filesystem
            //  he would return the wrong path.
            //
            //  Luckily, we can just use the mapping.
            //

            Name = &InstanceContext->Mapping.UserMapping.ShortNamePath.FinalComponentName;

        } else {

            //
            //  We have to query below us to get the short name.
            //

            Status = NcGetFileNameInformation( Data,
                                               FileObject,
                                               Instance,
                                               FLT_FILE_NAME_SHORT |
                                                   NameQueryMethod |
                                                   NameFlags,
                                               &ShortInfo );

            if (!NT_SUCCESS( Status )) {

                goto NcGenerateFileNameCleanup;
            }

            Status = FltParseFileNameInformation( ShortInfo );

            if (!NT_SUCCESS(Status)) {

                goto NcGenerateFileNameCleanup;
            }

            //
            //  Set name to returned name.
            //

            Name = &ShortInfo->Name;
        }
    }

    FLT_ASSERT( Name != NULL );

    //
    //  Try to grow the namechanger's record to accommodate the result.
    //

    Status = FltCheckAndGrowNameControl( OutputNameControl, Name->Length );

    if (NT_SUCCESS( Status )) {

        //
        //  Copy the new name into the buffer.
        //

        RtlCopyUnicodeString( &OutputNameControl->Name, Name );
        *CacheFileNameInformation = TRUE;
    }

NcGenerateFileNameCleanup:

    if (LowerNameInfo != NULL) {

        FltReleaseFileNameInformation( LowerNameInfo );
    }

    if (ShortInfo != NULL) {

        FltReleaseFileNameInformation( ShortInfo );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (MungedName.Buffer != NULL) {

        ExFreePoolWithTag( MungedName.Buffer, NC_GENERATE_NAME_TAG );
    }

    return Status;
}

NTSTATUS
NcNormalizeNameComponentEx (
    _In_     PFLT_INSTANCE            Instance,
    _In_opt_ PFILE_OBJECT             FileObject,
    _In_     PCUNICODE_STRING         ParentDirectory,
    _In_     USHORT                   DeviceNameLength,
    _In_     PCUNICODE_STRING         Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_     ULONG                    ExpandComponentNameLength,
    _In_     FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID           *NormalizationContext
    )
{

    NTSTATUS Status;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NC_PATH_OVERLAP ParentOverlap;    //overlap between the parent path and the user mapping.
    UNICODE_STRING Remainder;
    UNICODE_STRING MungedParentPath;  // Path we are going to open for query
    PCUNICODE_STRING MungedComponent; // File name to enumerate
    PWSTR MungedBuffer = NULL;
    ULONG MungedBufferLength;

    IO_STATUS_BLOCK ParentStatusBlock;
    OBJECT_ATTRIBUTES ParentAttributes;
    HANDLE ParentHandle = 0;
    PFILE_OBJECT ParentFileObject = NULL;
    BOOLEAN IgnoreCase = !BooleanFlagOn( Flags, FLTFL_NORMALIZE_NAME_CASE_SENSITIVE );

    PAGED_CODE();

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    UNREFERENCED_PARAMETER( NormalizationContext );
    UNREFERENCED_PARAMETER( DeviceNameLength );
    UNREFERENCED_PARAMETER( FileObject );

    Status = FltGetInstanceContext( Instance,
                                    &InstanceContext );

    if (!NT_SUCCESS( Status )) {

        goto NcNormalizeNameComponentExCleanup;
    }

    //
    //  Default to enumerating the component specified by the
    //  caller.
    //

    MungedComponent = Component;

    NcComparePath( ParentDirectory,
                   &InstanceContext->Mapping.UserMapping,
                   &Remainder,
                   IgnoreCase,
                   TRUE,
                   &ParentOverlap );

    //
    //  We need to figure out which path we are going to open.
    //

    if (ParentOverlap.InMapping) {

        //
        //  The parent is in the mapping, so it has to be
        //  munged in order to be opened.
        //

        MungedBufferLength = Remainder.Length + InstanceContext->Mapping.RealMapping.LongNamePath.FullPath.Length;

        MungedBuffer = ExAllocatePoolZero( PagedPool, MungedBufferLength, NC_NORMALIZE_NAME_TAG );

        if (MungedBuffer == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto NcNormalizeNameComponentExCleanup;
        }

        //
        //  Construct munged name.
        //

        MungedParentPath.Buffer = MungedBuffer;
        MungedParentPath.Length = 0;
        MungedParentPath.MaximumLength = (USHORT) MungedBufferLength;

        RtlCopyUnicodeString( &MungedParentPath,
                              &InstanceContext->Mapping.RealMapping.LongNamePath.FullPath );

        RtlAppendUnicodeStringToString( &MungedParentPath,
                                        &Remainder);

    } else if (ParentOverlap.Parent) {

        //
        //  The parent is the parent of the user mapping.
        //  That means we need to see if the final component is
        //  the mapping path itself.
        //

        if( RtlCompareUnicodeString( Component,
                                     &InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName,
                                     IgnoreCase) == 0 ||
            RtlCompareUnicodeString( Component,
                                     &InstanceContext->Mapping.UserMapping.ShortNamePath.FinalComponentName,
                                     IgnoreCase) == 0) {

            //
            //  The requested final component is the mapping itself.
            //  Hence the real mapping is the munged name.
            //

            MungedParentPath = InstanceContext->Mapping.RealMapping.LongNamePath.ParentPath;
            MungedComponent = &InstanceContext->Mapping.RealMapping.LongNamePath.FinalComponentName;

        } else {

            //
            //  The system is requesting another path in the user mapping's parent.
            //  Go ahead and open the given path.
            //

            MungedParentPath = *ParentDirectory;
        }

    } else { //ParentOverlap is not within or parent of mapping.

        MungedParentPath = *ParentDirectory;
    }

    //
    //  We should open MungedParentPath and enumerate it.
    //

    InitializeObjectAttributes( &ParentAttributes,
                                &MungedParentPath,
                                OBJ_KERNEL_HANDLE | (IgnoreCase?OBJ_CASE_INSENSITIVE:0),
                                NULL,
                                NULL);

    Status = NcCreateFileHelper( NcGlobalData.FilterHandle,
                                 Instance,
                                 &ParentHandle,
                                 &ParentFileObject,
                                 FILE_LIST_DIRECTORY | FILE_TRAVERSE,
                                 &ParentAttributes,
                                 &ParentStatusBlock,
                                 0,
                                 FILE_ATTRIBUTE_NORMAL,
                                 0,
                                 FILE_OPEN,
                                 FILE_DIRECTORY_FILE,
                                 NULL,
                                 0,
                                 IO_IGNORE_SHARE_ACCESS_CHECK,
                                 FileObject );

    if (!NT_SUCCESS( Status )) {

        goto NcNormalizeNameComponentExCleanup;
    }

    Status = NcQueryDirectoryFile( Instance,
                                   ParentFileObject,
                                   ExpandComponentName,
                                   ExpandComponentNameLength,
                                   FileNamesInformation,
                                   TRUE,
                                   (PUNICODE_STRING) MungedComponent,
                                   TRUE,
                                   NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcNormalizeNameComponentExCleanup;
    }

    //
    //  The object exists.  Now we need to return the correct
    //  final name.
    //

    if (Component != MungedComponent) {

        ULONG SizeRequired;

        SizeRequired = FIELD_OFFSET( FILE_NAMES_INFORMATION, FileName ) +
                       InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Length;

        if (ExpandComponentNameLength < SizeRequired) {

            Status = STATUS_BUFFER_OVERFLOW;
            goto NcNormalizeNameComponentExCleanup;
        }

        FLT_ASSERT( ExpandComponentName->NextEntryOffset == 0 );
        ExpandComponentName->NextEntryOffset = 0;
        ExpandComponentName->FileIndex = 0;
        ExpandComponentName->FileNameLength = InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Length;
        RtlCopyMemory( ExpandComponentName->FileName,
                       InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Buffer,
                       InstanceContext->Mapping.UserMapping.LongNamePath.FinalComponentName.Length );

    }

    //TODO THIS NEEDS TO

NcNormalizeNameComponentExCleanup:

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    if (MungedBuffer != NULL) {

        ExFreePoolWithTag( MungedBuffer, NC_NORMALIZE_NAME_TAG );
    }

    if (ParentHandle != 0) {

        FltClose( ParentHandle );
    }

    if (ParentFileObject != NULL) {

        ObDereferenceObject( ParentFileObject );

    }

    //TODO THERE ARE ONLY TWO VALID ERROR CODES: STATUS_NO_SUCH_FILE, STATUS_SUCCESS if you want the name construction to continue.
    //any other code will stop the query.
    return Status;
}
