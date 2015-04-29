
#include "nc.h"

_At_(OutputString->Buffer, _Post_notnull_)
NTSTATUS
NcLoadRegistryString (
    _In_ HANDLE Key,
    _In_ PCWSTR valueName,
    _Out_ PUNICODE_STRING OutputString
    );

BOOLEAN
NcIs8DOT3Compatible (
    _In_ PUNICODE_STRING TestName,
    _In_opt_ PUNICODE_STRING LongName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, NcInitializeMapping)
#pragma alloc_text(INIT, NcLoadRegistryString)
#pragma alloc_text(INIT, NcIs8DOT3Compatible)
#endif

//  The #pragma is a notation to the static code analyzer that the Buffer
//  returned from the function will always be properly initialized.
//  The multiple allocations/frees of the buffers causes it to lose track.
#pragma warning(push)
#pragma warning(disable:6001)
_At_(OutputString->Buffer, _Post_notnull_)
NTSTATUS
NcLoadRegistryString (
    _In_ HANDLE Key,
    _In_ PCWSTR valueName,
    _Out_ PUNICODE_STRING OutputString
    )
{
#pragma warning(pop)
    PKEY_VALUE_PARTIAL_INFORMATION TempMappingBuffer = NULL;
    ULONG TempMappingKeyLength = 0;
    UNICODE_STRING ValueString;
    PWCHAR OutputStringBuffer = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Query the length of the registry value.
    //
    
    RtlInitUnicodeString( &ValueString, valueName );

NcLoadRegistryStringRetry:
    Status = ZwQueryValueKey( Key,
                              &ValueString,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &TempMappingKeyLength );

    //
    //  If we could not successfully locate the value, return the
    //  error to our caller.
    //

    if (Status != STATUS_BUFFER_TOO_SMALL &&
        Status != STATUS_BUFFER_OVERFLOW) {

        goto NcLoadRegistryStringCleanup;
    }

    //
    //  Allocate a buffer large enough to hold the string.
    //

    if (TempMappingBuffer != NULL) {
        ExFreePoolWithTag( TempMappingBuffer, NC_TAG );
    } 

    TempMappingBuffer = ExAllocatePoolWithTag( PagedPool,
                                               TempMappingKeyLength,
                                               NC_TAG );

    if (TempMappingBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcLoadRegistryStringCleanup;
    }

    //
    //  Now attempt to read the string.
    //

    Status = ZwQueryValueKey( Key,
                              &ValueString,
                              KeyValuePartialInformation,
                              TempMappingBuffer,
                              TempMappingKeyLength,
                              &TempMappingKeyLength );

    //
    //  If the value is changing underneath us, the length we
    //  collected above may be stale.  Loop back, reallocate
    //  and try again.
    //

    if (Status == STATUS_BUFFER_TOO_SMALL ||
        Status == STATUS_BUFFER_OVERFLOW) {

        goto NcLoadRegistryStringRetry;
    }

    if (!NT_SUCCESS( Status )) {

        goto NcLoadRegistryStringCleanup;
    }

    //
    //  If we're reading a string, it had better:
    //  1. Be a string.
    //  2. Fit in a UNICODE_STRING.
    //  3. Have some characters in it (we never need empty strings in this filter.)
    //

    if (TempMappingBuffer->Type != REG_SZ ||
        TempMappingBuffer->DataLength >= MAXUSHORT ||
        TempMappingBuffer->DataLength <= sizeof(WCHAR)) {

        Status = STATUS_INVALID_PARAMETER;
        goto NcLoadRegistryStringCleanup;
    }

    //
    //  Allocate a buffer for the target string.  Note that we
    //  allocate one fewer WCHAR, as we have no need for the 
    //  NULL terminator in our UNICODE_STRING.
    //

    OutputStringBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                                TempMappingBuffer->DataLength - sizeof(WCHAR),
                                                NC_TAG );

    if (OutputStringBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcLoadRegistryStringCleanup;
    }

    //
    //  We only modify the output string on success.  On failure, it is
    //  left with previous values.
    //

    Status = STATUS_SUCCESS;

    OutputString->MaximumLength = (USHORT)TempMappingBuffer->DataLength - sizeof(WCHAR);
    OutputString->Buffer = OutputStringBuffer;

    RtlCopyMemory( OutputStringBuffer, TempMappingBuffer->Data, OutputString->MaximumLength );
    OutputString->Length = OutputString->MaximumLength;

    //
    //  This buffer is in use and should not be cleaned up.
    //

    OutputStringBuffer = NULL;

NcLoadRegistryStringCleanup:

    if (TempMappingBuffer != NULL) {
        ExFreePoolWithTag( TempMappingBuffer, NC_TAG );
    }

    if (OutputStringBuffer != NULL) {
        ExFreePoolWithTag( OutputStringBuffer, NC_TAG );
    }

    return Status;
}

BOOLEAN
NcIs8DOT3Compatible (
    _In_ PUNICODE_STRING TestName,
    _In_opt_ PUNICODE_STRING LongName
    )
{
    BOOLEAN SpacesPresent;
    USHORT Index;
    PAGED_CODE();

    //
    //  When the user supplies a shortname, we expect it to be a valid
    //  shortname.  This function will check the name's length.
    //

    if (!RtlIsNameLegalDOS8Dot3( TestName,
                                 NULL,
                                 &SpacesPresent )) {

        return FALSE;

    }

    //
    //  Our shortnames should not have spaces.
    //

    if (SpacesPresent) {
        return FALSE;
    }

    //
    //  In this sample, we enforce that the shortname must NOT contain
    //  a tilde (~).
    //
    //  If the shortname could contain a tilde, the filesystem would
    //  be able to autogenerate a conflicting shortname in response to
    //  an operation on a long name.  We could only detect this
    //  afterwards (in a post-operation callback), but we may not be
    //  able to handle the condition correctly.  Explicitly changing
    //  a shortname requires NTFS and restore privilege.  Rather than
    //  attempt to obtain this functionality via creative mechanism,
    //  this filter simply prevents a shortname which could create
    //  this condition.
    //
    //  Note that in a product it may be advantageous to create both
    //  sides of the mapping on disk.  Part of this sample is to
    //  illustrate the emulation of an object which does not exist,
    //  so we did not do this here.
    //
    //  We enforce:
    //  1. The name must not have a tilde (for the above reasons);
    //  2. The name must not contain a path seperator;
    //  3. The name must be fully uppercase to be a valid DOS name
    //

    for (Index = 0;
         Index < TestName->Length/sizeof(WCHAR);
         Index++) {

        if (TestName->Buffer[Index] == L'~' ||
            TestName->Buffer[Index] == L'\\' ||
            TestName->Buffer[Index] != RtlUpcaseUnicodeChar( TestName->Buffer[Index] )) {

            return FALSE;

        }
    }

    //
    //  We're done validating the short name.  Now we must check the
    //  long and short names for consistency.  If we have no long name,
    //  we're done.
    //

    if (!ARGUMENT_PRESENT( LongName )) {
        return TRUE;
    }

    //
    //  Check if the long name is a valid shortname.  We recurse into
    //  ourselves for this.  Note that since we're not specifying a
    //  long name, the recursion is bounded.
    //

    if (NcIs8DOT3Compatible( LongName, NULL )) {

        //
        //  If both our long and short paths are compliant, they had
        //  better be the same.
        //

        if (!RtlEqualUnicodeString( TestName, LongName, FALSE )) {
            return FALSE;
        }
    }

    return TRUE;

}

NTSTATUS 
NcInitializeMapping(
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Descrition:

    This routine initializes the mapping structure. It will
    try to populate it from the registry, and if that fails
    use a default string.

Arguments:

    RegistryPath - The path key passed to the driver during DriverEntry.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DriverRegKey = NULL;
    UNICODE_STRING TempPath = EMPTY_UNICODE_STRING;
    USHORT Index;

    PAGED_CODE();

    RtlZeroMemory( &NcGlobalData, sizeof( NcGlobalData ));

    //
    //  Open the mapping registry key.
    //

    InitializeObjectAttributes( &Attributes,
                                RegistryPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = ZwOpenKey( &DriverRegKey,
                        KEY_READ,
                        &Attributes );

    if (!NT_SUCCESS( Status )) {

        FLT_ASSERT( DriverRegKey == NULL );
        goto NcInitializeMappingCleanup;
    }

    Status = NcLoadRegistryString( DriverRegKey,
                                   L"UserMapping",
                                   &TempPath );

    if (!NT_SUCCESS( Status )) {
        goto NcInitializeMappingCleanup;
    }

    //
    //  We check that the name does not contain two
    //  contiguous slashes.  This implies an empty
    //  name component, which we make no attempt to
    //  handle.
    //

    for (Index = 1; Index < TempPath.Length/sizeof(WCHAR); Index++) {

        if (TempPath.Buffer[Index] == L'\\' &&
            TempPath.Buffer[Index - 1] == L'\\') {

            Status = STATUS_INVALID_PARAMETER;
            goto NcInitializeMappingCleanup;

        }
    }

    Status = NcParseFinalComponent( &TempPath,
                                    &NcGlobalData.UserMappingPath,
                                    &NcGlobalData.UserMappingFinalComponentLong );

    if (!NT_SUCCESS( Status )) {
        goto NcInitializeMappingCleanup;
    }

    NcFreeUnicodeString( &TempPath );

    Status = NcLoadRegistryString( DriverRegKey,
                                   L"UserMappingFinalComponentShort",
                                   &NcGlobalData.UserMappingFinalComponentShort );

    if (!NT_SUCCESS( Status )) {
        goto NcInitializeMappingCleanup;
    }

    Status = NcLoadRegistryString( DriverRegKey,
                                   L"RealMapping",
                                   &TempPath );

    if (!NT_SUCCESS( Status )) {
        goto NcInitializeMappingCleanup;
    }

    //
    //  We check that the name does not contain two
    //  contiguous slashes.  This implies an empty
    //  name component, which we make no attempt to
    //  handle.
    //

    for (Index = 1; Index < TempPath.Length/sizeof(WCHAR); Index++) {

        if (TempPath.Buffer[Index] == L'\\' &&
            TempPath.Buffer[Index - 1] == L'\\') {

            Status = STATUS_INVALID_PARAMETER;
            goto NcInitializeMappingCleanup;

        }
    }

    Status = NcParseFinalComponent( &TempPath,
                                    &NcGlobalData.RealMappingPath,
                                    &NcGlobalData.RealMappingFinalComponent );

    if (!NT_SUCCESS( Status )) {
        goto NcInitializeMappingCleanup;
    }

    //
    //  We expect out parent mappings to start with '\', and we expect
    //  no final component to start with '\'.  We have already checked
    //  that the strings contain one WCHAR, but we require two for the
    //  parent mappings, since the first one is '\'.
    //

    if (NcGlobalData.RealMappingPath.Length < (2 * sizeof(WCHAR)) ||
        NcGlobalData.UserMappingPath.Length < (2 * sizeof(WCHAR)) ||
        NcGlobalData.RealMappingPath.Buffer[0] != L'\\' ||
        NcGlobalData.UserMappingPath.Buffer[0] != L'\\' ||
        NcGlobalData.UserMappingFinalComponentShort.Buffer[0] == L'\\' ||
        NcGlobalData.UserMappingFinalComponentLong.Buffer[0] == L'\\' ||
        NcGlobalData.RealMappingFinalComponent.Buffer[0] == L'\\') {

        Status = STATUS_INVALID_PARAMETER;
        goto NcInitializeMappingCleanup;
    }

    if (!NcIs8DOT3Compatible( &NcGlobalData.UserMappingFinalComponentShort,
                              &NcGlobalData.UserMappingFinalComponentLong )) {
        
        Status = STATUS_INVALID_PARAMETER;
        goto NcInitializeMappingCleanup;
    }

    //
    //  TODO: This sample assumes that the real mapping final component
    //  is short.  This should not be a required assumption.  Note that
    //  since we only have one component for the real mapping (long and
    //  short), we don't need to check the long name for compatibility
    //  with the short name.
    //

    if (!NcIs8DOT3Compatible( &NcGlobalData.RealMappingFinalComponent, NULL )) {
        
        Status = STATUS_INVALID_PARAMETER;
        goto NcInitializeMappingCleanup;
    }

NcInitializeMappingCleanup:


    if (!NT_SUCCESS( Status )) {

        if (NcGlobalData.UserMappingPath.Buffer != NULL) {
            NcFreeUnicodeString( &NcGlobalData.UserMappingPath );
        }

        if (NcGlobalData.UserMappingFinalComponentShort.Buffer != NULL) {
            NcFreeUnicodeString( &NcGlobalData.UserMappingFinalComponentShort );
        }

        if (NcGlobalData.UserMappingFinalComponentLong.Buffer != NULL) {
            NcFreeUnicodeString( &NcGlobalData.UserMappingFinalComponentLong );
        }

        if (NcGlobalData.RealMappingPath.Buffer != NULL) {
            NcFreeUnicodeString( &NcGlobalData.RealMappingPath );
        }

        if (NcGlobalData.RealMappingFinalComponent.Buffer != NULL) {
            NcFreeUnicodeString( &NcGlobalData.RealMappingFinalComponent );
        }
    }

    if (TempPath.Buffer != NULL) {
        NcFreeUnicodeString( &TempPath );
    }

    if (DriverRegKey != NULL) {

        NTSTATUS BogusStatus;

        BogusStatus = ZwClose( DriverRegKey );
        FLT_ASSERT(NT_SUCCESS( BogusStatus ));
    }

    return Status;
}

