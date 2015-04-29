#include "nc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcComparePath)
#pragma alloc_text(PAGE, NcConstructPath)
#pragma alloc_text(PAGE, NcParseFinalComponent)
#endif

#define NcIsCharComponentTerminator( C ) \
    ((C) == L'\\' || (C) == L':')

BOOLEAN
NcComparePath (
    _In_ PCUNICODE_STRING Name,
    _In_ PNC_MAPPING_ENTRY Mapping,
    _Out_opt_ PUNICODE_STRING Remainder,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN ContainsDevice,
    _Out_ PNC_PATH_OVERLAP Overlap
    )
/*++

Routine Description:

    Compares a name against a mapping and returns the Overlap.

Arguments:

    Name - The name we are checking.

    Mapping - The mapping path we are comparing against.

    Remainder - If the string matches the mapping, then Remainder will be 
                the portion of the name after the mapping to the end of the
                string.

    IgnoreCase - Case sensitivity.

    ContainsDevice - TRUE if the input name is expected to contain a fully
                     qualified path including the device name.  FALSE if the
                     path is relative to the root of the volume (excludes
                     device.)

    Overlap - Pointer to an NC_PATH_OVERLAP structure.  Structure members will
              be set when this routine exits.

Return Value:

    TRUE if the string is in the mapping; Remainder is written to if present.
    FALSE if the string is not in the mapping; Remainder is not written to.

    Note that this is not a success code; this routine always succeeds.  Further
    information about the mapping is in the Overlap parameter.

--*/
{
    PUNICODE_STRING LongName;
    PUNICODE_STRING ShortName;

    PWSTR NameBuff;
    PWSTR LongBuff;
    PWSTR ShortBuff;

    WCHAR NameBuffCur;
    WCHAR LongBuffCur;
    WCHAR ShortBuffCur;

    USHORT NameIndex;                    // Our index in Name
    USHORT NameMatchIndex = 0;               // Our index of matched components in Name
    USHORT LongIndex;                    // Our index in MappingPath->LongNamePath
    USHORT ShortIndex;                   // Our index in MappingPath->ShortNamePath

    BOOLEAN LongMatch;                   // TRUE if long name component matches
    BOOLEAN ShortMatch;                  // TRUE if short name component matches

    USHORT NameLength;                   // Max index for NameIndex
    USHORT LongLength;
    USHORT ShortLength;

    BOOLEAN NameDone;                    // TRUE when we hit a '\',stop comparing for this component.
    BOOLEAN LongDone;
    BOOLEAN ShortDone;

    USHORT ComponentMatches = 0;         // Number of component matches.
    USHORT MappingComponents;
    USHORT VolumeComponents;

    PAGED_CODE();

    if (ContainsDevice) {
        LongName = &Mapping->LongNamePath.FullPath;
        ShortName = &Mapping->ShortNamePath.FullPath;

        MappingComponents = Mapping->LongNamePath.NumberComponentsInFullPath;
        VolumeComponents = Mapping->LongNamePath.NumberComponentsInVolumePath;
    } else {
        LongName = &Mapping->LongNamePath.VolumelessName;
        ShortName = &Mapping->ShortNamePath.VolumelessName;

        MappingComponents = Mapping->LongNamePath.NumberComponentsInFullPath - Mapping->LongNamePath.NumberComponentsInVolumePath;
        VolumeComponents = 0;
    }

    NameBuff = Name->Buffer;
    LongBuff = LongName->Buffer;
    ShortBuff = ShortName->Buffer;

    NameLength = Name->Length/sizeof(WCHAR);     // Max index for NameIndex
    LongLength = LongName->Length/sizeof(WCHAR);
    ShortLength = ShortName->Length/sizeof(WCHAR);

    //
    //  Names are Unicode strings, so the number of bytes in
    //  the length should always be even.
    //

    FLT_ASSERT( Name->Length % sizeof(WCHAR) == 0 );
    FLT_ASSERT( LongName->Length % sizeof(WCHAR) == 0 );
    FLT_ASSERT( ShortName->Length % sizeof(WCHAR) == 0 );
    FLT_ASSERT( Mapping->LongNamePath.NumberComponentsInFullPath == Mapping->ShortNamePath.NumberComponentsInFullPath );

    //  Set initial values
    NameIndex = 0;
    LongIndex = 0;
    ShortIndex = 0;
    NameMatchIndex = 0;

    do //  Loop for component scan
    {

        //
        //  Mark that we have not reached the end of the component.
        //  Once we reach end of component, we stop going forward until everyone has caught up.
        //

        NameDone = FALSE;
        LongDone = FALSE;
        ShortDone = FALSE;

        //
        //  Mark that the names match
        //  We assume they match until we prove otherwise.
        //

        LongMatch = TRUE;
        ShortMatch = TRUE;

        do //  Loop for character scan.
        {

            //
            //  See if we have reached the end of the component.
            //

            NameDone = (BOOLEAN)(NameIndex >= NameLength || NcIsCharComponentTerminator( NameBuff[NameIndex] ));
            LongDone = (BOOLEAN)(LongIndex >= LongLength || LongBuff[LongIndex] == '\\');
            ShortDone = (BOOLEAN)(ShortIndex >= ShortLength || ShortBuff[ShortIndex] == '\\');

            if (NameDone) {

                //
                //  If name has run off end, or hit end of component,
                //  then we need to break.
                //

                break;
            }

            if(LongDone && ShortDone) {

                //
                //  If the paths we are comparing against have
                //  both ether run out or hit the end of their 
                //  respective components, then we need to break.
                //

                break;
            }

            //
            //  Convert characters into case insensitive mode (if needed)
            //

            if (IgnoreCase) {

                NameBuffCur = RtlUpcaseUnicodeChar( NameBuff[NameIndex] );
                if (!LongDone) {
                    LongBuffCur = RtlUpcaseUnicodeChar( LongBuff[LongIndex] );
                }

                if (!ShortDone) {
                    ShortBuffCur = RtlUpcaseUnicodeChar( ShortBuff[ShortIndex] );
                }

            } else {

                NameBuffCur = NameBuff[NameIndex];
                if (!LongDone) {
                    LongBuffCur = LongBuff[LongIndex];
                }

                if (!ShortDone) {
                    ShortBuffCur = ShortBuff[ShortIndex];
                }
            }

            //
            //  Compare characters to verify match.
            //

            if (!LongDone) {
                if (NameBuffCur != LongBuffCur) {
                    LongMatch = FALSE; // If the characters are not matches, then we are not a match.
                }
            } else {
                LongMatch = FALSE; // If we're comparing beyond the end of the buffer, we don't match.
            }

            if (!ShortDone) {
                if (NameBuffCur != ShortBuffCur) {
                    ShortMatch = FALSE; // If the characters are not matches, then we are not a match.
                }
            } else {
                ShortMatch = FALSE;
            }

            //
            //  Move to next index
            //

            NameIndex++;
            if (!LongDone) {
                LongIndex++;
            }
            if (!ShortDone) {
                ShortIndex++;
            }

            //
            //  Loop if there is still chance of match 
            //

        } while(LongMatch || ShortMatch);//  end of character scan.

        //
        //  We scanned until we had 2 mismatches or we ran off the end of the name,
        //  or we ran off the end of the long and short names.
        //  We should scan everyone forward until they are all off the end of their 
        //  buffer, or it the end of their component.
        //

        if (NameIndex < NameLength && !NcIsCharComponentTerminator( NameBuff[NameIndex] )) {

            //
            //  We broke out of character comparison, but were not at the end
            //  of the name's component. This means that both the long and short
            //  names were shorter than the name's component. Thus they cannot
            //  be matches. Furthermore, because neither of them are matches, we
            //  can stop the search.
            //

            LongMatch = FALSE;
            ShortMatch = FALSE;
            break;
        }

        while (NameIndex < NameLength && NameBuff[NameIndex] != '\\') {

            //
            //  We may still consider this component a match due to a stream.
            //  In this case, we need to advance the name index until we
            //  find the next slash.
            //
            //  TODO: Note one effect of this is we will believe
            //  \dir1\foo matches \dir1:Stream\foo.  The latter name is not a
            //  valid name on any Microsoft filesystem.
            //

            NameIndex++;
        }

        while( LongIndex < LongLength && LongBuff[LongIndex] != '\\' ) {

            //
            //  We were not at the end of the long name's component.
            //  So it cannot be a match.
            //  Scan forward until we find the end.
            //

            LongMatch = FALSE;
            LongIndex++;        
        }
            
        while( ShortIndex < ShortLength && ShortBuff[ShortIndex] != '\\' ) {

            //
            //  We were not at the end of the short name's component.
            //  So it cannot be a match.
            //  Scan forward until we find the end.
            //
            
            ShortMatch = FALSE;
            ShortIndex++;
        }

        if ((LongMatch || ShortMatch) && NameIndex != 0) {
            ComponentMatches++;
        }

        //
        //  All the indexes should be at the end of their buffer or their component.
        //

        FLT_ASSERT( NameIndex == NameLength || NameBuff[NameIndex] == '\\' );
        FLT_ASSERT( LongIndex == LongLength || LongBuff[LongIndex] == '\\' );
        FLT_ASSERT( ShortIndex == ShortLength || ShortBuff[ShortIndex] == '\\' );

        //
        //  Since we are all lined up on '\', lets move forward to next component...
        //

        if (NameIndex < NameLength) {
            NameIndex++;
        }

        if (LongIndex < LongLength) {
            LongIndex++;
        }

        if (ShortIndex < ShortLength) {
            ShortIndex++;
        }

        if (LongMatch || ShortMatch) {
            NameMatchIndex = NameIndex;
        }

        //
        //  Keep looping if name's last component matched either 
        //  the long or short's component and name and the mapping
        //  have more components.
        //
        //  NOTE: if LongIndex < LongLength,
        //  then ShortIndex < ShortLength because they have the same number of components.
        //

        FLT_ASSERT( (LongIndex < LongLength) ? (ShortIndex < ShortLength) : TRUE );

    } while( (LongMatch || ShortMatch) && NameIndex < NameLength && LongIndex < LongLength );

    //
    //  Now we need to figure out how far we made it, and
    //  apply the appropriate flags.
    //

    Overlap->EntireFlags = 0;

    //
    //  We can't match more components than exist or something's seriously wrong.
    //
    FLT_ASSERT( ComponentMatches <= MappingComponents );

    if (ComponentMatches >= MappingComponents &&
        NameLength > NameMatchIndex) {

        //
        //  If we've matched all the components in the mapping and still have data
        //  left over, we're in the mapping.
        //

        Overlap->InMapping = TRUE;

        if (Remainder != NULL) {

            Remainder->Buffer = &NameBuff[NameMatchIndex];
            Remainder->Length = (NameLength - NameMatchIndex) * sizeof(WCHAR);
            Remainder->MaximumLength = Remainder->Length;
        }

    } else if (ComponentMatches == MappingComponents) {

        //
        //  If we matched all the components in the mapping and have nothing
        //  left over, we are the mapping.
        //

        Overlap->Match = TRUE;
        Overlap->InMapping = TRUE;

        if (Remainder != NULL) {

            Remainder->Buffer = &NameBuff[NameIndex];
            Remainder->MaximumLength = Remainder->Length = 0;
        }

    } else if (ComponentMatches == MappingComponents - 1 &&
               NameLength > NameMatchIndex) {

        //
        //  If we matched everything except the final component but have data
        //  left over, we may be a peer of the mapping.  For this to be true,
        //  the only string left must be a filename, not a path.
        //

        Overlap->Peer = TRUE;

        for (;NameIndex < NameLength;NameIndex++) {
            if (NameBuff[NameIndex] == L'\\') {
                Overlap->Peer = FALSE;
                break;
            }
        }


    } else if (ComponentMatches == MappingComponents - 1) {

        //
        //  If we matched everything except the final component, then we must
        //  be the parent.
        //

        Overlap->Parent = TRUE;
        Overlap->Ancestor = TRUE;

    } else if (ComponentMatches >= VolumeComponents &&
               NameLength == NameMatchIndex) {

        //
        //  If we matched something and have no data left over, then we are
        //  an ancestor path.
        //

        Overlap->Ancestor = TRUE;
    }

    return (BOOLEAN)Overlap->InMapping;
}

_Post_satisfies_(NewName->MaximumLength < MAXUSHORT)
_Post_satisfies_(NewName->Length <= NewName->MaximumLength)
_Must_inspect_result_
NTSTATUS
NcConstructPath (
    _In_ PNC_MAPPING_ENTRY RealPath,
    _In_ PUNICODE_STRING Remainder,
    _In_ BOOLEAN IncludeVolume,
    _Out_ _At_(NewName->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING NewName
    )
/*++

Routine Description:

    Constructs a path

Arguments:

    RealPath - Path from the mapping we want to use in name generation.

    Remainder - Remainder string generated by NcComparePath.

    IncludeVolume - If TRUE, the volume will be prepended to the name.

    NewName - Output name.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT NameLength;
    PWCHAR NameBuffer;
    USHORT SeparatorLength;

    PAGED_CODE();

    //
    //  Calculate length of combined name.
    //

    SeparatorLength = (Remainder->Length == 0 ? 0 : sizeof(WCHAR));
    NameLength = RealPath->LongNamePath.VolumelessName.Length + SeparatorLength + Remainder->Length;
    if( IncludeVolume ) {

        NameLength = NameLength + RealPath->LongNamePath.VolumePath.Length;
    }

    //
    //  Potentially a file may exist on disk which is less than MAXUSHORT,
    //  but our mapping changes the length such that it now exceeds
    //  MAXUSHORT.  We can't really handle this object meaningfully, since
    //  all path operations have a UNICODE_STRING limitation.  Accordingly,
    //  some files may be inaccessible for some purposes as a result of
    //  this.  In practice it won't happen, and the important thing is to
    //  ensure we don't overflow and create security vulnerabilities.
    //
    if (NameLength >= MAXUSHORT) {

        Status = STATUS_OBJECT_PATH_INVALID;
        goto NcConstructPathCleanup;
    }

    //
    //  Allocate space for combined name.
    //

    NameBuffer = ExAllocatePoolWithTag( PagedPool,
                                        NameLength,
                                        NC_GENERATE_NAME_TAG );

    if (NameBuffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcConstructPathCleanup;
    }

    //
    //  Zero out destination 
    //

    NewName->Buffer = NameBuffer;
    NewName->Length = 0;
    NewName->MaximumLength = NameLength;

    //
    //  Copy Volume Path
    //

    if (IncludeVolume) {

        Status = RtlAppendUnicodeStringToString( NewName, &RealPath->LongNamePath.VolumePath );

        FLT_ASSERT( Status == STATUS_SUCCESS );
    }

    //
    //  Copy mapping path.
    //

    Status = RtlAppendUnicodeStringToString( NewName,
                                             &RealPath->LongNamePath.VolumelessName );

    FLT_ASSERT( Status == STATUS_SUCCESS );

    //
    //  Append separator
    //

    if (SeparatorLength > 0) {

        NewName->Buffer[NewName->Length/sizeof(WCHAR)] = NC_SEPARATOR;
        NewName->Length += sizeof(WCHAR);

        FLT_ASSERT( NewName->Length <= NewName->MaximumLength );
    }

    //
    //  Append Remainder.
    //

    Status = RtlAppendUnicodeStringToString( NewName, Remainder );

    FLT_ASSERT( Status == STATUS_SUCCESS );

NcConstructPathCleanup:

    return Status;
}

NTSTATUS
NcParseFinalComponent(
    _In_ PUNICODE_STRING EntirePath,
    _Out_ PUNICODE_STRING ParentPath,
    _Out_ PUNICODE_STRING FinalComponent
    )
{
    USHORT Index = EntirePath->Length / sizeof(WCHAR);
    USHORT ParentLength;
    USHORT FinalComponentLength;
    BOOLEAN FoundFinalComponent = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR ParentStringBuffer = NULL;
    PWSTR FinalStringBuffer = NULL;

    PAGED_CODE();

    while(Index > 0) {
        Index--;
        if (EntirePath->Buffer[Index] == L'\\') {
            FoundFinalComponent = TRUE;
            break;
        }
    }

    //
    //  Paths should contain at least one seperator.
    //  We expect all paths to be absolute, relative to
    //  the volume, so they should always start with '\'.
    //
    if (!FoundFinalComponent) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Typically we don't want to include a trailing
    //  slash in the parent path.  The exception to this
    //  rule is for the volume root.
    //
    if (Index > 1) {
        ParentLength = Index * sizeof(WCHAR);
    } else {
        ParentLength = (Index + 1) * sizeof(WCHAR);
    }

    //
    //  We also don't want to have no final component.
    //  This implies a user configured the path as
    //  \a\b\c\, including the trailing slash.
    //
    FinalComponentLength = EntirePath->Length - (Index + 1) * sizeof(WCHAR);
    if (FinalComponentLength == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    ParentStringBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                                ParentLength,
                                                NC_TAG );

    if (ParentStringBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcParseFinalComponentCleanup;
    }

    FinalStringBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                               FinalComponentLength,
                                               NC_TAG );

    if (FinalStringBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto NcParseFinalComponentCleanup;
    }

    RtlCopyMemory( ParentStringBuffer, EntirePath->Buffer, ParentLength );

    ParentPath->Buffer = ParentStringBuffer;
    ParentPath->MaximumLength = 
    ParentPath->Length = ParentLength;

    RtlCopyMemory( FinalStringBuffer, &EntirePath->Buffer[Index + 1], FinalComponentLength );

    FinalComponent->Buffer = FinalStringBuffer;
    FinalComponent->MaximumLength =
    FinalComponent->Length = FinalComponentLength;

    //
    //  We've completed successfully, and our allocated
    //  buffers are in use.
    //
    ParentStringBuffer = NULL;
    FinalStringBuffer = NULL;

NcParseFinalComponentCleanup:

    if (ParentStringBuffer) {
        ExFreePoolWithTag( ParentStringBuffer, NC_TAG );
    }

    if (FinalStringBuffer) {
        ExFreePoolWithTag( FinalStringBuffer, NC_TAG );
    }

    return Status;
}
