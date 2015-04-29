/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    ncmapping.c

Abstract:

    This module contains helper routines for manipulating mapping
    objects.

Environment:

    Kernel mode

--*/

#include "nc.h"

BOOLEAN
NcIsMappingPathZeroed (
    _In_ PNC_MAPPING_PATH Path
    );

NTSTATUS
NcBuildMappingPath (
    _In_ PUNICODE_STRING VolumeName,
    _In_ PUNICODE_STRING ParentPath,
    _In_ PUNICODE_STRING FinalComponent,
    _Inout_ PNC_MAPPING_PATH Path
    );

NTSTATUS
NcBuildMappingPathFromFile (
    _In_ PFILE_OBJECT Parent,
    _In_ PUNICODE_STRING FinalComponent,
    _In_ PFLT_INSTANCE Instance,
    _In_ BOOLEAN Normalized,
    _Inout_ PNC_MAPPING_PATH Path
    );


//
//  Functions to manage mapping entry
//

BOOLEAN
NcIsMappingEntryZeroed (
    PNC_MAPPING_ENTRY Entry
    );

VOID
NcInitMappingEntry (
    _Inout_ PNC_MAPPING_ENTRY Entry
    );

VOID
NcTeardownMappingEntry (
    _Inout_ PNC_MAPPING_ENTRY Path
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcBuildMappingPath)
#pragma alloc_text(PAGE, NcBuildMappingPathFromFile)
#pragma alloc_text(PAGE, NcBuildMappingPathFromVolume)
#pragma alloc_text(PAGE, NcInitMappingPath)
#pragma alloc_text(PAGE, NcIsMappingPathZeroed)
#pragma alloc_text(PAGE, NcTeardownMappingPath)
#pragma alloc_text(PAGE, NcInitMappingEntry)
#pragma alloc_text(PAGE, NcIsMappingEntryZeroed)
#pragma alloc_text(PAGE, NcTeardownMappingEntry)
#pragma alloc_text(PAGE, NcInitMapping)
#pragma alloc_text(PAGE, NcIsMappingZeroed)
#pragma alloc_text(PAGE, NcTeardownMapping)
#pragma alloc_text(PAGE, NcBuildMapping)
#endif

//
//  Functions to manage the mapping path.
//

BOOLEAN
NcIsMappingPathZeroed(
    _In_ PNC_MAPPING_PATH Path
    )
{
    PAGED_CODE();

    if (Path->FullPath.Buffer != NULL ||
        Path->FullPath.Length != 0 ||
        Path->FullPath.MaximumLength != 0) {

        return FALSE;
    }

    if (Path->VolumePath.Buffer != NULL ||
        Path->VolumePath.Length != 0 ||
        Path->VolumePath.MaximumLength != 0) {
    
        return FALSE;
    }

    if (Path->ParentPath.Buffer != NULL ||
        Path->ParentPath.Length != 0 ||
        Path->ParentPath.MaximumLength != 0) {

        return FALSE;
    }

    if (Path->FinalComponentName.Buffer != NULL ||
        Path->FinalComponentName.Length != 0 ||
        Path->FinalComponentName.MaximumLength != 0) {

        return FALSE;
    }

    if (Path->VolumelessName.Buffer != NULL ||
        Path->VolumelessName.Length != 0 ||
        Path->VolumelessName.MaximumLength != 0) {

        return FALSE;
    }

    return TRUE;
}

VOID
NcInitMappingPath (
    _Out_ PNC_MAPPING_PATH Path
    )
/*++

Routine Description: 

    Routine to initialize a mapping path.

Arguments:

    Path - Pointer to a user allocated NC_MAPPING_PATH.

Return Value:

    None.

--*/
{
    PAGED_CODE();
    RtlZeroMemory( Path, sizeof( NC_MAPPING_PATH ) );
}

VOID
NcTeardownMappingPath (
    _Inout_ PNC_MAPPING_PATH Path
    )
/*++

Routine Description:

    Frees the allocations in a NC_MAPPING_PATH.

Arguments:
    
    Path - The mapping which you want to clean up.

Return Value:
    
    None.

--*/
{

    PAGED_CODE();

    if (Path->FullPath.Buffer != NULL) {

        ExFreePoolWithTag( Path->FullPath.Buffer, NC_MAPPING_TAG );
    }

    //
    //  All of the strings point at the same buffer allocation.
    //  So once we free the buffer in the FullPath, we just zero out
    //  all of the other strings.
    //

    NcInitMappingPath( Path );
}

NTSTATUS
NcBuildMappingPath (
    _In_ PUNICODE_STRING VolumeName,
    _In_ PUNICODE_STRING ParentPath,
    _In_ PUNICODE_STRING FinalComponent,
    _Inout_ PNC_MAPPING_PATH Path
    )
/*++

Routine Description:

    Builds a NC_MAPPING_PATH from a collection of Unicode strings.

Arguments:

    VolumeName - Name of the volume. 

    ParentPath - string for the parent of mapping.

    FinalComponent - The final component of the mapping path.

    Path - Pointer to a used allocated and zeroed NC_MAPPING_PATH.

Return Value:

    Returns STATUS_SUCCESS on success. Otherwise returns an error code.


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT NameLength;
    PWCHAR NameBuffer = NULL;
    UNICODE_STRING NameString;
    USHORT SeparatorLength;
    WCHAR ParentEnd;
    USHORT Index;

    PAGED_CODE();

    FLT_ASSERT( NcIsMappingPathZeroed( Path ) );

    //
    //  We cannot remap to/from the root of the volume.
    //

    FLT_ASSERT( FinalComponent->Length > 0 );

    //
    //  If the parent path does not end with a '\' then we insert one
    //  between the parent and the final component.
    //

    ParentEnd = ParentPath->Buffer[(ParentPath->Length / sizeof(WCHAR)) - 1];

    if (ParentEnd != NC_SEPARATOR) {

        SeparatorLength = sizeof(WCHAR);
    } else {

        SeparatorLength = 0;
    }

    //
    //  Allocate Buffer for Name.
    //

    NameLength = VolumeName->Length + ParentPath->Length + SeparatorLength + FinalComponent->Length;

    NameBuffer = ExAllocatePoolWithTag( PagedPool,
                                        NameLength,
                                        NC_MAPPING_TAG );

    if (NameBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcBuildMappingPathCleanup;
    }

    //
    //  Create String
    //

    NameString.Length = 0;
    NameString.MaximumLength = NameLength;
    NameString.Buffer = NameBuffer;

    //
    //  Copy Volume Name
    //

    RtlCopyUnicodeString( &NameString, VolumeName );

    //
    //  Copy Parent Path
    //

    RtlAppendUnicodeStringToString( &NameString,ParentPath );

    //
    //  Add separator
    //

    if (SeparatorLength != 0) {
        
        NameString.Buffer[NameString.Length/SeparatorLength] = NC_SEPARATOR;
        NameString.Length = NameString.Length + SeparatorLength;
        FLT_ASSERT( NameString.Length <= NameString.MaximumLength );
    }

    //
    //  Copy Final Component
    //

    RtlAppendUnicodeStringToString( &NameString, FinalComponent );

    //
    //  Setup Unicode Strings
    //

    Path->FullPath.Buffer = NameString.Buffer;
    Path->FullPath.Length = NameString.Length;
    Path->FullPath.MaximumLength = NameString.MaximumLength;

    Path->VolumePath.Buffer = NameString.Buffer;
    Path->VolumePath.Length = VolumeName->Length;
    Path->VolumePath.MaximumLength = VolumeName->Length;

    Path->ParentPath.Buffer = NameString.Buffer;
    Path->ParentPath.Length = SeparatorLength == sizeof(WCHAR) ? 
        VolumeName->Length+ParentPath->Length : 
        VolumeName->Length+ParentPath->Length-sizeof(WCHAR);
    Path->ParentPath.MaximumLength = SeparatorLength == sizeof(WCHAR) ? 
        VolumeName->Length+ParentPath->Length : 
        VolumeName->Length+ParentPath->Length-sizeof(WCHAR);

    Path->FinalComponentName.Buffer = (PWSTR)Add2Ptr( NameString.Buffer,
                                                      VolumeName->Length+ParentPath->Length+SeparatorLength );

    Path->FinalComponentName.Length = FinalComponent->Length;
    Path->FinalComponentName.MaximumLength = FinalComponent->Length;

    Path->VolumelessName.Buffer = (PWSTR)Add2Ptr( NameString.Buffer, VolumeName->Length );
    Path->VolumelessName.Length = NameString.Length - VolumeName->Length;
    Path->VolumelessName.MaximumLength = Path->VolumelessName.Length;

    Path->NumberComponentsInVolumePath = 0;

    for (Index = 0; Index < Path->VolumePath.Length/sizeof(WCHAR); Index++) {
        if (Path->VolumePath.Buffer[Index] == L'\\') {
            Path->NumberComponentsInVolumePath++;
        }
    }

    Path->NumberComponentsInFullPath = 0;

    for (Index = 0; Index < Path->FullPath.Length/sizeof(WCHAR); Index++) {
        if (Path->FullPath.Buffer[Index] == L'\\') {
            Path->NumberComponentsInFullPath++;
        }
    }

    FLT_ASSERT( Path->NumberComponentsInFullPath > Path->NumberComponentsInVolumePath );

    //
    //  We reached the end without incident.
    //
     
    Status = STATUS_SUCCESS;

NcBuildMappingPathCleanup:

    if (!NT_SUCCESS( Status )) {

        if (NameBuffer != NULL) {
            ExFreePoolWithTag( NameBuffer, NC_MAPPING_TAG );
        }

        NcInitMappingPath( Path );
    }

    return Status;
}

NTSTATUS
NcBuildMappingPathFromVolume (
    _In_ PFLT_VOLUME CONST Volume,
    _In_ PUNICODE_STRING ParentPath,
    _In_ PUNICODE_STRING FinalComponentName,
    _Inout_ PNC_MAPPING_PATH Entry
    )
/*++

Routine Description:

    Constructs a path from a Volume and a Caller Specified Path.

Arguments:

    Volume - Pointer to the volume which the mapping path will be used on.

    ParentPath - Path to the parent of the mapping.

    FinalComponentName - The final component name.

    Entry - Pointer to a user allocated NC_MAPPING_PATH.

Return Value:

    Returns STATUS_SUCCESS on success. Otherwise returns an error code.

--*/
{
    NTSTATUS Status;
    ULONG VolumeNameLength;
    PVOID VolumeNameBuffer = NULL;
    UNICODE_STRING VolumeNameString;

    PAGED_CODE();

    FLT_ASSERT( NcIsMappingPathZeroed( Entry ) );

    //
    //  Query the volume name length.
    //

    Status = FltGetVolumeName( Volume,
                               NULL,
                               &VolumeNameLength);

    if (!NT_SUCCESS( Status ) && Status != STATUS_BUFFER_TOO_SMALL) {

        goto NcBuildPathCleanup;
    } 

    //
    //  Allocate a buffer for the name.
    //

    Status = STATUS_SUCCESS;

    VolumeNameBuffer = ExAllocatePoolWithTag( PagedPool,
                                              VolumeNameLength,
                                              NC_MAPPING_TAG );

    if (VolumeNameBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcBuildPathCleanup;
    }

    RtlInitEmptyUnicodeString( &VolumeNameString, VolumeNameBuffer, (USHORT) VolumeNameLength );

    //
    //  Query the volume name
    //

    Status = FltGetVolumeName( Volume,
                               &VolumeNameString,
                               NULL );

    if (!NT_SUCCESS( Status )) {

        goto NcBuildPathCleanup;
    }

    //
    //  Generate Mapping Path
    //

    Status = NcBuildMappingPath( &VolumeNameString,
                                 ParentPath,
                                 FinalComponentName,
                                 Entry );

    if (!NT_SUCCESS( Status )) {

        goto NcBuildPathCleanup;
    }

NcBuildPathCleanup:

    if (!NT_SUCCESS( Status )) {

        NcTeardownMappingPath( Entry );
    }

    if (VolumeNameBuffer != NULL) {

        ExFreePoolWithTag( VolumeNameBuffer, NC_MAPPING_TAG );
    }

    return Status;
}

NTSTATUS
NcBuildMappingPathFromFile (
    _In_ PFILE_OBJECT Parent,
    _In_ PUNICODE_STRING FinalComponent,
    _In_ PFLT_INSTANCE Instance,
    _In_ BOOLEAN Normalized,
    _Inout_ PNC_MAPPING_PATH Path
    )
/*++

Routine Description:

    Builds a path from the parent of the mapping and string for the final component.

Arguments:

    Parent - The file object for the parent of the mapping.

    FinalComponent - A string which is to be used as the final component of the name.

    Instance - The instance for which we are building the mapping.

    NameFlags - Parameters which we will use when checking the name from the file object.

    Path - Pointer to a user allocated NC_MAPPING_PATH which will be populated.

Return Value 

    On success, returns STATUS_SUCCESS, otherwise returns an error code.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING ParentPath;
    PFLT_FILE_NAME_INFORMATION ParentNameInfo = NULL;
    FLT_FILE_NAME_OPTIONS NameFlags;

    PAGED_CODE();

    FLT_ASSERT( NcIsMappingPathZeroed( Path ) );

    if (Normalized) {

        NameFlags = FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT;

    } else {

        NameFlags = FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT;

    }

    //
    //  Get File parent's name info.
    //

    Status = NcGetFileNameInformation( NULL, 
                                       Parent,
                                       Instance,
                                       NameFlags,
                                       &ParentNameInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcBuildMappingPathFromFileCleanup;
    }

    Status = FltParseFileNameInformation( ParentNameInfo );

    if (!NT_SUCCESS( Status )) {

        goto NcBuildMappingPathFromFileCleanup;
    }


    FLT_ASSERT( ParentNameInfo->Format == FLT_FILE_NAME_NORMALIZED || 
                ParentNameInfo->Format == FLT_FILE_NAME_OPENED );


    //
    //  Format Parent Name
    //

    ParentPath.Buffer = ParentNameInfo->ParentDir.Buffer;
    ParentPath.Length = ParentNameInfo->ParentDir.Length+ParentNameInfo->FinalComponent.Length;
    ParentPath.MaximumLength = ParentPath.Length;

    //
    //  Generate name
    //

    Status = NcBuildMappingPath( &ParentNameInfo->Volume,
                                 &ParentPath,
                                 FinalComponent,
                                 Path );

NcBuildMappingPathFromFileCleanup:

    if (!NT_SUCCESS( Status )) {

        NcTeardownMappingPath( Path );
    }

    if (ParentNameInfo != NULL) {

        FltReleaseFileNameInformation( ParentNameInfo );
    }

    return Status;
}

//
//  Functions to manage mapping entry
//

BOOLEAN
NcIsMappingEntryZeroed (
    PNC_MAPPING_ENTRY Entry
    )
{
    PAGED_CODE();

    if (!NcIsMappingPathZeroed( &Entry->LongNamePath ))
        return FALSE;

    if (!NcIsMappingPathZeroed( &Entry->ShortNamePath ))
        return FALSE;

    return TRUE;
}

VOID
NcInitMappingEntry (
    _Inout_ PNC_MAPPING_ENTRY Entry
    )
{
    PAGED_CODE();

    NcInitMappingPath( &Entry->LongNamePath );
    NcInitMappingPath( &Entry->ShortNamePath );
}

VOID
NcTeardownMappingEntry (
    _Inout_ PNC_MAPPING_ENTRY Path
    )
{
    PWCHAR StrBuff = Path->LongNamePath.FullPath.Buffer;

    PAGED_CODE();

    NcTeardownMappingPath( &Path->LongNamePath );

    if (StrBuff != Path->ShortNamePath.FullPath.Buffer) {

        //
        //  There are situations where the two paths point to the
        //  same buffer. We do this check to make sure that
        //  we don't double free it.
        //

        NcTeardownMappingPath( &Path->ShortNamePath );

    } else {

        NcInitMappingPath( &Path->ShortNamePath );
    }
}

//
//  Functions to manage mapping
//

BOOLEAN
NcIsMappingZeroed (
    PNC_MAPPING Mapping
    )
{
    PAGED_CODE();
    if (!NcIsMappingEntryZeroed( &Mapping->RealMapping )) {

        return FALSE;
    }

    if (!NcIsMappingEntryZeroed( &Mapping->UserMapping )) {

        return FALSE;
    }

    return TRUE;
}

VOID
NcInitMapping (
    PNC_MAPPING Mapping
    )
{
    PAGED_CODE();
    NcInitMappingEntry( &Mapping->RealMapping );
    NcInitMappingEntry( &Mapping->UserMapping );
}

VOID
NcTeardownMapping ( 
    _Inout_ PNC_MAPPING Mapping
    )
{
    PAGED_CODE();
    NcTeardownMappingEntry( &Mapping->RealMapping );
    NcTeardownMappingEntry( &Mapping->UserMapping );
}

NTSTATUS
NcBuildMapping (
    _In_ PFILE_OBJECT UserParent,
    _In_ PFILE_OBJECT RealParent,
    _In_ PUNICODE_STRING UserFinalComponentShortName,
    _In_ PUNICODE_STRING UserFinalComponentLongName,
    _In_ PUNICODE_STRING RealFinalComponentName,
    _In_ PFLT_INSTANCE Instance,
    _Out_ PNC_MAPPING Mapping
    )
/*++

Routine Description:

    Constructs the mapping for instance setup.

Arguments:

    UserParent - File object of the parent of the user mapping.

    RealParent - File object of the parent of the real mapping.

    UserFinalComponentShortName - Short name for the user mapping's final component.

    UserFinalComponentLongName - Long name for the user mapping's final component.

    RealFinalComponentName - Final component name for the real mapping (either long or short).

    Instance - Instance for which we are constructing the mapping.

    Mapping - Pointer to a user allocated NC_MAPPING which will be populated.

--*/
{

    NTSTATUS Status;
    PAGED_CODE();

    FLT_ASSERT( NcIsMappingZeroed( Mapping ) );

    //
    //  Build the Real Path.
    //

    Status = NcBuildMappingPathFromFile( RealParent,
                                         RealFinalComponentName,
                                         Instance,
                                         TRUE,
                                         &Mapping->RealMapping.LongNamePath );

    if (!NT_SUCCESS( Status )) {

        goto NcBuildMappingCleanup;
    }

    //  TODO: Do we really know that the real path is 8dot3 compliant?

    //
    //  Reuse the real path for long and short name.
    //

    Mapping->RealMapping.ShortNamePath =
        Mapping->RealMapping.LongNamePath;

    //
    //  Build the user short path.
    //

    Status = NcBuildMappingPathFromFile( UserParent,
                                         UserFinalComponentShortName,
                                         Instance,
                                         FALSE,
                                         &Mapping->UserMapping.ShortNamePath );

    if (!NT_SUCCESS( Status )) {

        goto NcBuildMappingCleanup;
    }

    //
    //  Build the user long path.
    //

    Status = NcBuildMappingPathFromFile( UserParent,
                                         UserFinalComponentLongName,
                                         Instance,
                                         TRUE,
                                         &Mapping->UserMapping.LongNamePath );

    if (!NT_SUCCESS( Status )) {

        goto NcBuildMappingCleanup;
    }

NcBuildMappingCleanup:

    return Status;
}


