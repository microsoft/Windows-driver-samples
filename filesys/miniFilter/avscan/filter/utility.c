/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    utility.c

Abstract:

    Utility module implementation.
    1) Generic table routines
    2) Query file information routines

Environment:

    Kernel mode

--*/

#include "avscan.h"

//
//  Generic table routines.
//

RTL_GENERIC_COMPARE_RESULTS
NTAPI
AvCompareEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Lhs,
    _In_ PVOID Rhs
    )
/*++

Routine Description:

    This routine is the callback for the generic table routines.

Arguments:

    Table       - Table for which this is invoked.

    FirstStruct - An element in the table to compare.

    SecondStruct - Another element in the table to compare.

Return Value:

    RTL_GENERIC_COMPARE_RESULTS.

--*/
{
    PAV_GENERIC_TABLE_ENTRY lhs = (PAV_GENERIC_TABLE_ENTRY)Lhs;
    PAV_GENERIC_TABLE_ENTRY rhs = (PAV_GENERIC_TABLE_ENTRY)Rhs;

    UNREFERENCED_PARAMETER (Table);

    //
    //  Compare the 128 bit fileId in 64bit pieces for efficiency.
    //  Compare the lower 64 bits Value first since that is used
    //  in both 128 bit and 64 bit fileIds and doing so eliminates
    //  and unnecessary comparison of the UpperZeros field in the
    //  most common case. Note this comparison is not equivalent
    //  to a memcmp on the 128 bit values but that doesn't matter
    //  here since we just need the tree to be self-consistent.
    //

    if (lhs->FileId.FileId64.Value < rhs->FileId.FileId64.Value) {

        return GenericLessThan;

    } else if (lhs->FileId.FileId64.Value > rhs->FileId.FileId64.Value) {

        return GenericGreaterThan;

    } else if (lhs->FileId.FileId64.UpperZeroes < rhs->FileId.FileId64.UpperZeroes) {

        return GenericLessThan;

    } else if (lhs->FileId.FileId64.UpperZeroes > rhs->FileId.FileId64.UpperZeroes) {

        return GenericGreaterThan;
    }

    return GenericEqual;
}


PVOID
NTAPI
AvAllocateGenericTableEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ CLONG ByteSize
    )
/*++

Routine Description:

    This routine is the callback for allocation for entries in the generic table.

Arguments:

    Table       - Table for which this is invoked.

    ByteSize    - Amount of memory to allocate.

Return Value:

    Pointer to allocated memory if successful, else NULL.

--*/
{

    UNREFERENCED_PARAMETER (Table);

    return ExAllocatePoolZero( PagedPool, ByteSize, AV_TABLE_ENTRY_TAG );
}

VOID
NTAPI
AvFreeGenericTableEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Entry
    )
/*++

Routine Description:

    This routine is the callback for releasing memory for entries in the generic
    table.

Arguments:

    Table       - Table for which this is invoked.

    Entry       - Entry to free.

Return Value:

    None.

--*/
{

    UNREFERENCED_PARAMETER (Table);

    ExFreePoolWithTag( Entry, AV_TABLE_ENTRY_TAG );
}

//
//  Query File Information Routines
//

NTSTATUS
AvGetFileId (
    _In_    PFLT_INSTANCE Instance,
    _In_    PFILE_OBJECT FileObject,
    _Out_   PAV_FILE_REFERENCE FileId
    )
/*++

Routine Description:

    This routine obtains the File ID and saves it in the stream context.

Arguments:

    Instance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.

    FileObject - File object pointer for the file. This parameter is required and cannot be NULL.

    pFileId - Pointer to file id. This is the output

Return Value:

    Returns statuses forwarded from FltQueryInformationFile.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    FLT_FILESYSTEM_TYPE type;

    //
    //  Querying for FileInternalInformation gives you the file ID.
    //

    status = FltGetFileSystemType( Instance, &type );

    if (NT_SUCCESS( status )) {

        if (type == FLT_FSTYPE_REFS) {

            FILE_ID_INFORMATION fileIdInformation;

            status = FltQueryInformationFile( Instance,
                                              FileObject,
                                              &fileIdInformation,
                                              sizeof(FILE_ID_INFORMATION),
                                              FileIdInformation,
                                              NULL );

            if (NT_SUCCESS( status )) {

                RtlCopyMemory(&(FileId->FileId128), &(fileIdInformation.FileId), sizeof(FileId->FileId128) );
            }

        } else {

            FILE_INTERNAL_INFORMATION fileInternalInformation;

            status = FltQueryInformationFile( Instance,
                                              FileObject,
                                              &fileInternalInformation,
                                              sizeof(FILE_INTERNAL_INFORMATION),
                                              FileInternalInformation,
                                              NULL );

            if (NT_SUCCESS( status )) {

                FileId->FileId64.Value = fileInternalInformation.IndexNumber.QuadPart;
                FileId->FileId64.UpperZeroes = 0ll;
            }
        }
    }

    return status;
}

NTSTATUS
AvGetFileSize (
    _In_    PFLT_INSTANCE Instance,
    _In_    PFILE_OBJECT FileObject,
    _Out_   PLONGLONG Size
    )
/*++

Routine Description:

    This routine obtains the size.

Arguments:

    Instance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.

    FileObject - File object pointer for the file. This parameter is required and cannot be NULL.

    Size - Pointer to a LONGLONG indicating the file size. This is the output.

Return Value:

    Returns statuses forwarded from FltQueryInformationFile.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    FILE_STANDARD_INFORMATION standardInfo;

    //
    //  Querying for FileStandardInformation gives you the offset of EOF.
    //

    status = FltQueryInformationFile( Instance,
                                      FileObject,
                                      &standardInfo,
                                      sizeof(FILE_STANDARD_INFORMATION),
                                      FileStandardInformation,
                                      NULL );

    if (NT_SUCCESS( status )) {

        *Size = standardInfo.EndOfFile.QuadPart;
    }

    return status;
}

NTSTATUS
AvGetFileEncrypted (
    _In_   PFLT_INSTANCE Instance,
    _In_   PFILE_OBJECT FileObject,
    _Out_  PBOOLEAN  Encrypted
    )
/*++

Routine Description:

    This routine obtains the File ID and saves it in the stream context.

Arguments:

    Instance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.

    FileObject - File object pointer for the file. This parameter is required and cannot be NULL.

    Encrypted - Pointer to a boolean indicating if this file is encrypted or not. This is the output.

Return Value:

    Returns statuses forwarded from FltQueryInformationFile.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    FILE_BASIC_INFORMATION basicInfo;

    //
    //  Querying for basic information to get encryption.
    //

    status = FltQueryInformationFile( Instance,
                                      FileObject,
                                      &basicInfo,
                                      sizeof(FILE_BASIC_INFORMATION),
                                      FileBasicInformation,
                                      NULL );

    if (NT_SUCCESS( status )) {

        *Encrypted = BooleanFlagOn( basicInfo.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
    }

    return status;
}

LONG
AvExceptionFilter (
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
    //  Certain exceptions shouldn't be dismissed within the filter
    //  unless we're touching user memory.
    //

    if (!FsRtlIsNtstatusExpected( Status ) &&
        !AccessingUserBuffer) {

        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


