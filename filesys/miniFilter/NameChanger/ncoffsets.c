#include "nc.h"

/*

Munging entries from the directory query responses requires name changer
to interpret the data returned from the query. There are currently 9 
information classes, each with a different structure for holding the file data.

Of the 9 queries, we are interested in the following:

FileBothDirectoryInformation
FileDirectoryInformation
FileFullDirectoryInformation
FileFullDirectoryInformation
FileIdFullDirectoryInformation
FileNamesInformation
FileObjectIdInformation

Each of structures related to these queries have the following properties:
1) Holds the offset to the next entry
2) Holds the file name length (in bytes)
3) The first WCHAR of the file name is the last entry of the structure,
with the remainder of the name following the structure.

The purpose of the DIRECTORY_CONTROL_OFFSETS structure is to allow
a generic method of interpreting the query data by holding the offset
information for each of the above variables. Combined with the directory
walking functions (below) we now have an easy uniform means of parsing
the directory queries.

*/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcCalculateDirectoryNotificationOffsets)
#pragma alloc_text(PAGE, NcDetermineStructureOffsets)
#pragma alloc_text(PAGE, NcGetEntrySize)
#pragma alloc_text(PAGE, NcGetFileName)
#pragma alloc_text(PAGE, NcGetFileNameLength)
#pragma alloc_text(PAGE, NcGetNextEntry)
#pragma alloc_text(PAGE, NcGetNextEntryOffset)
#pragma alloc_text(PAGE, NcGetShortName)
#pragma alloc_text(PAGE, NcGetShortNameLength)
#pragma alloc_text(PAGE, NcSetFileName)
#pragma alloc_text(PAGE, NcSetNextEntryOffset)
#pragma alloc_text(PAGE, NcSetShortName)
#endif

//---------------------------------------------------------------------------
// FUNCTIONS FOR WALKING DIRECTORY STRUCTURE
//---------------------------------------------------------------------------
BOOLEAN
NcDetermineStructureOffsets (
    _Out_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ FILE_INFORMATION_CLASS Information
    )
/*++

Routine Description:

    Determines the structure offsets for the FILE_INFORMATION_CLASS provided.

Arguments:

    Offsets - Output DIRECTORY_CONTROL_OFFSETS pointer.

    Information - What structure to use.

    DirectoryBuffer - A sample buffer for us to test with.

Returns:

    return TRUE if we have a structure to handle this request,
    otherwise return FALSE. Offsets are returned through the Offsets
    pointer.

--*/
{
    BOOLEAN ReturnValue = TRUE;
    PAGED_CODE();

    //
    //  Not all directory information classes have short names. 
    //  So we'll zero out short name data for classes that don't expose it .
    //

    Offsets->ShortNamePresent = FALSE;
    Offsets->ShortNameLengthDist = 0;
    Offsets->ShortNameDist = 0;
    
    switch( Information ) {

        case FileBothDirectoryInformation:

            Offsets->NextEntryOffsetDist =
                FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, NextEntryOffset );

            Offsets->FileNameLengthDist =
                FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileNameLength );

            Offsets->FileNameDist =
                FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileName );

            Offsets->ShortNamePresent = TRUE;

            Offsets->ShortNameLengthDist =
                FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, ShortNameLength );

            Offsets->ShortNameDist = 
                FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, ShortName );

            break;

        case FileDirectoryInformation:

            Offsets->NextEntryOffsetDist =
                FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, NextEntryOffset );

            Offsets->FileNameLengthDist =
                FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileNameLength );

            Offsets->FileNameDist =
                FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileName );

            break;

        case FileFullDirectoryInformation:

            Offsets->NextEntryOffsetDist =
                FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, NextEntryOffset );

            Offsets->FileNameLengthDist =
                FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileNameLength );

            Offsets->FileNameDist =
                FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileName );

            break;

        case FileIdBothDirectoryInformation:

            Offsets->NextEntryOffsetDist =
                FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION, NextEntryOffset );

            Offsets->FileNameLengthDist =
                FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION, FileNameLength );

            Offsets->FileNameDist =
                FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION, FileName );

            Offsets->ShortNamePresent = TRUE;

            Offsets->ShortNameLengthDist =
                FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION, ShortNameLength );

            Offsets->ShortNameDist = 
                FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION, ShortName );

            break;

        case FileIdFullDirectoryInformation:

            Offsets->NextEntryOffsetDist =
                FIELD_OFFSET( FILE_ID_FULL_DIR_INFORMATION, NextEntryOffset );

            Offsets->FileNameLengthDist =
                FIELD_OFFSET( FILE_ID_FULL_DIR_INFORMATION, FileNameLength );

            Offsets->FileNameDist =
                FIELD_OFFSET( FILE_ID_FULL_DIR_INFORMATION, FileName );

            break;

        case FileNamesInformation:

            Offsets->NextEntryOffsetDist =
                FIELD_OFFSET( FILE_NAMES_INFORMATION, NextEntryOffset );

            Offsets->FileNameLengthDist =
                FIELD_OFFSET( FILE_NAMES_INFORMATION, FileNameLength );

            Offsets->FileNameDist =
                FIELD_OFFSET( FILE_NAMES_INFORMATION, FileName );

            break;

        default:

            //
            //  There are queries that we don't filter,
            //  so just return FALSE to state failure.
            //

            ReturnValue = FALSE;
            break;
    }

    return ReturnValue;
}

VOID
NcCalculateDirectoryNotificationOffsets (
    PDIRECTORY_CONTROL_OFFSETS Offsets 
    )
/*++

Routine Description:

    Directory Notifications use a structure with similar properties 
    to a directory query. CalculateDirectoryNotificationOffsets allows
    us to reuse the ncoffsets library with directory notification structures.

Arguments

    Offsets - Output DIRECTORY_CONTROL_OFFSETS pointer.

--*/
{

    PAGED_CODE();

    //
    //  Place offsets for next entry and file name.
    //
    
    Offsets->NextEntryOffsetDist = FIELD_OFFSET( FILE_NOTIFY_INFORMATION, NextEntryOffset );
    Offsets->FileNameLengthDist = FIELD_OFFSET( FILE_NOTIFY_INFORMATION, FileNameLength );
    Offsets->FileNameDist = FIELD_OFFSET( FILE_NOTIFY_INFORMATION, FileName );

    //
    //  Directory notification does not have short names.
    //

    Offsets->ShortNamePresent = FALSE;
    Offsets->ShortNameLengthDist = 0;
    Offsets->ShortNameDist = 0;
}


ULONG
NcGetNextEntryOffset (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    )
/*++

Routine Description:

    Returns the offset in bytes to the next entry.

Arguments:

    Buffer - Pointer to the start of the current entry.

    Offsets - Offsets structure for this information class.

Return Value:

    The number of bytes from Buffer to the next entry.

--*/
{
    PULONG Offset = Add2Ptr( Buffer, Offsets->NextEntryOffsetDist );
    PAGED_CODE();
    return *Offset;
}


PVOID
NcGetNextEntry (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    )
/*++

Routine Description:

    Returns a pointer to the next entry in the buffer.

Arguments:

    Buffer - Pointer to the start of the current entry.

    Offsets - Offsets structure for this information class.

Return Value:

    Returns a pointer to the beginning of the next entry.

--*/
{
    ULONG Offset = NcGetNextEntryOffset( Buffer, Offsets );
    PVOID NextEntry = Add2Ptr( Buffer, Offset );
    PAGED_CODE();
    return NextEntry;
}


ULONG
NcGetFileNameLength (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    )
/*++

Routine Description:

    Returns the length of the file name in bytes.

Arguments:

    Buffer - Pointer to the start of the entry.

    Offsets - Offsets structure for the information class.

Return Value:

    Returns the length of the file name component of the buffer.

--*/
{
    ULONG Result = *((PULONG)(Add2Ptr( Buffer, Offsets->FileNameLengthDist )));
    PAGED_CODE();
    return Result;
}


ULONG
NcGetEntrySize (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    )
/*++

Routine Description:

    Returns the size of this entry in bytes.

Arguments:

    Buffer - Pointer to the start of the entry.

    Offsets - Offsets structure for the information class.

Return Value:

    Returns the size of this entry in bytes.

--*/
{
    ULONG EntrySize = NcGetNextEntryOffset(Buffer, Offsets);
    PAGED_CODE();

    if (EntrySize == 0) {

        //
        //  We are at last entry, so we need to calculate this ourselves.
        //

        EntrySize = NcGetFileNameLength( Buffer, Offsets );
        EntrySize += Offsets->FileNameDist;
    }

    return EntrySize;
}


PWSTR
NcGetFileName (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    )
/*++

Routine Description:

    Returns a pointer to the file name string.

Arguments:

    Buffer - Pointer to the start of the entry.

    Offsets - Offsets structure for the information class.

Return Value:

    A pointer to the start of the file name for this entry.

--*/
{
    PAGED_CODE();
    return Add2Ptr(Buffer, Offsets->FileNameDist);
}


PWSTR
NcGetShortName (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    )
/*++

Routine Description:

    Returns a pointer to the short name for this entry.

Arguments:

    Buffer - Pointer to the start of this entry.

    Offsets - Offsets structure for the information class.

Return Value:

    If there is a short name in this information class, returns a pointer to the
    start of the short name. Otherwise, returns NULL.
--*/
{
    PAGED_CODE();

    if (Offsets->ShortNamePresent) {

        return Add2Ptr(Buffer, Offsets->ShortNameDist);

    } else {

        return NULL;
    }
}


CCHAR
NcGetShortNameLength (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    )
/*++

Routine Description:

    Returns the length of the shortname.

Arguments

    Buffer - Pointer to the start of this entry.

    Offsets - Offsets structure for the information class.

Return Value 

    Returns the length of the short name.
    Returns -1 on failure.
    
--*/
{
    CCHAR *Ptr;
    PAGED_CODE();

    if (Offsets->ShortNamePresent) {

        Ptr = Add2Ptr(Buffer, Offsets->ShortNameLengthDist);
        return *Ptr;
    } else {

        return -1;
    }
}


VOID
NcSetNextEntryOffset (
    _Inout_ PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ BOOLEAN ForceLast
    )
/*++

Routine Description:

    Sets the Next Entry Offset to Value.

Arguments:

    Buffer - Pointer to the start of this entry.

    Offsets - Offsets structure for the information class.

    ForceLast - If true, the size of the next entry is set to zero.
                Otherwise, the size will be set to the size of the 
                information class + the file name's length.

--*/
{
    //
    //  Get pointer to NextEntryOffset value.
    //
    
    PULONG NextEntry = Add2Ptr( Buffer, Offsets->NextEntryOffsetDist );
    PAGED_CODE();
 
    //
    //  Get length of name.
    //
    
    if (ForceLast) {

        //
        //  This is the last entry, so we need to make sure 0 is in the 
        //  next entry offset.
        //

        *NextEntry = 0;

    } else {

        //
        //  Next entry offset is the distance to the name
        //  plus the length of the name.
        //
        
        ULONG NameLength = NcGetFileNameLength( Buffer, Offsets );
        *NextEntry = Offsets->FileNameDist + NameLength;
    }
}


VOID 
NcSetFileName (
    _In_ PVOID Entry,
    _In_ PWSTR NewName,
    _In_ ULONG Length,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ BOOLEAN ForceLast
    )
/*++

Routine Description:

    Sets a new file name into the entry.

Arguments:

    Entry - A pointer to the start of the entry.

    NewName - A pointer to the new file name.

    Length - The length of the new name (in bytes).

    Offsets - Offsets structure for the information class.

    ForceLast - If true, the entry's size will be set to zero
                so that it looks like a valid last entry.

Return Value

    None.

--*/
{
    PWSTR NamePtr;
    PULONG NameLength;

    PAGED_CODE();

    //
    //  Get a pointer to the name in the buffer.
    //

    NamePtr = NcGetFileName( Entry, Offsets);
    NameLength = Add2Ptr( Entry, Offsets->FileNameLengthDist );

    //
    //  Copy the new name into buffer.
    //
    
    RtlCopyMemory( NamePtr, NewName, Length );
    *NameLength = Length;

    //
    //  Now we have to update the size of this entry.
    //

    NcSetNextEntryOffset( Entry, Offsets, ForceLast );
}


VOID 
NcSetShortName (
    _In_ PVOID Entry,
    _In_ PWSTR NewShortName,
    _In_ USHORT Length,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    )
/*++

Routine Description:

    Sets a new short name into an entry if the information class
    supports short names.

Arguments:

    Entry - Pointer to the start of an entry.

    NewShortName - Pointer to the new shortname.

    Length - The length of the short name in bytes.

    Offsets - Offsets structure for the information class.

Return Value:

    None.

--*/
{
    PWSTR NamePtr;
    PCCHAR NameLength;

    PAGED_CODE();

    if( Offsets->ShortNamePresent ) {

        NamePtr = NcGetShortName( Entry, Offsets );
        NameLength = Add2Ptr( Entry, Offsets->ShortNameLengthDist );

        FLT_ASSERT( Length <= (12 * sizeof(WCHAR)) );

        RtlCopyMemory(NamePtr, NewShortName, Length );
        *NameLength = (UCHAR) Length;
    }
}

