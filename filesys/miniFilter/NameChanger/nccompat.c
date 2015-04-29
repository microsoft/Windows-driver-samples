/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    nccompat.c

Abstract:

    Contains compatibility routines so that name changer works on
    all OSes.  Functions not available on older OSes are emulated here.
    We try, wherever possible, to call into newer routines directly and
    only fall back to emulation if required.

Environment:

    Kernel mode

--*/

#include "nc.h"

//
//  Prototypes for alternates.
//

NTSTATUS
NcReplaceFileObjectNameAlternate (
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(FileNameLength) PWSTR NewFileName,
    _In_ USHORT FileNameLength
    );

NTSTATUS
NcQueryDirectoryFileAlternate (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan,
    _Out_opt_ PULONG LengthReturned
    );

NTSTATUS
FLTAPI
NcCreateFileEx2Alternate (
    _In_ PFLT_FILTER Filter,
    _In_opt_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE FileHandle,
    _Outptr_opt_ PFILE_OBJECT *FileObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Flags,
    _In_opt_ PIO_DRIVER_CREATE_CONTEXT DriverContext
    );

#pragma alloc_text(PAGE, NcCompatInit)
#pragma alloc_text(PAGE, NcCreateFileEx2Alternate)
#pragma alloc_text(PAGE, NcQueryDirectoryFileAlternate)
#pragma alloc_text(PAGE, NcReplaceFileObjectNameAlternate)

//
//  Global variables which hold pointers to the functions we will call.
//

NC_REPLACE_FILEOBJECT_NAME_TYPE NcReplaceFileObjectName;
NC_QUERY_DIRECTORY_FILE_TYPE NcQueryDirectoryFile;
NC_CREATE_FILE_EX2_TYPE NcCreateFileEx2;
NC_GET_NEW_SYSTEM_BUFFER_ADDRESS NcGetNewSystemBufferAddress;

//
//  Holder for FltCreateFileEx, if it exists and we need to call it.
//

NC_CREATE_FILE_EX_TYPE NcCreateFileEx;

//
//  Alternative routines which are used when on downlevel os'es.
//

NTSTATUS
NcReplaceFileObjectNameAlternate (
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(FileNameLength) PWSTR NewFileName,
    _In_ USHORT FileNameLength
    )
/*++

Routine Description:

    This routine is used to replace a file object's name with a provided
    name. On Win7 and forward IoReplaceFileObjectName will be used.
    If this function is used and verifier is enabled on pre Win7 machines
    the filter will fail to unload due to a false positive on the leaked
    pool test.

Arguments:

    FileObject - Pointer to file object whose name is to be replaced.

    NewFileName - Pointer to buffer containing the new name.

    FileNameLength - Length of the new name in bytes.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES - No memory to allocate the new buffer.

    STATUS_SUCCESS otherwise.

--*/
{  
    PWSTR Buffer;
    PUNICODE_STRING FileName;
    USHORT NewMaxLength;

    PAGED_CODE();

    FileName = &FileObject->FileName;

    //
    //  If the new name fits inside the current buffer we simply copy it over
    //  instead of allocating a new buffer (and keep the MaximumLength value
    //  the same).
    //

    if (FileNameLength <= FileName->MaximumLength) {

        goto CopyAndReturn;
    }

    NewMaxLength = FileNameLength;

    Buffer = ExAllocatePoolWithTag( PagedPool,
                                    NewMaxLength,
                                    NC_FILE_NAME_TAG );

    if (!Buffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (FileName->Buffer != NULL) {

        ExFreePool(FileName->Buffer);
    }

    FileName->Buffer = Buffer;
    FileName->MaximumLength = NewMaxLength;

CopyAndReturn:

    RtlZeroMemory(FileName->Buffer, FileName->MaximumLength);

    FileName->Length = FileNameLength;
    RtlCopyMemory(FileName->Buffer, NewFileName, FileNameLength);

    return STATUS_SUCCESS;
}

NTSTATUS
NcQueryDirectoryFileAlternate (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan,
    _Out_opt_ PULONG LengthReturned
    )
/*++

Routine Description:

    This function issues a directory control operation to the filesystem

Arguments:


    Instance - Supplies the Instance initiating this IO.

    FileObject - Supplies the file object about which the requested
        Information should be queried.

    FileInformation - Supplies a buffer to hold the directory control
        results.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specifies the file Information class requested.

    ReturnSingleEntry - Retrieve only one file at a time.

    FileName - This is the pattern to search for.

    RestartScan - Restart the directory enumeration from the start.

    LengthReturned - On success, this is how much Information was placed
        into the FileInformation buffer.

Return Value:

    The status returned is the final completion Status of the operation.

--*/
{
    PFLT_CALLBACK_DATA Info;
    NTSTATUS Status;

    PAGED_CODE();

    Status = FltAllocateCallbackData( Instance, FileObject, &Info );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    Info->Iopb->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    Info->Iopb->MinorFunction = IRP_MN_QUERY_DIRECTORY;

    Info->Iopb->Parameters.DirectoryControl.QueryDirectory.Length = Length;
    Info->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName = FileName;
    Info->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass = FileInformationClass;
    Info->Iopb->Parameters.DirectoryControl.QueryDirectory.FileIndex = 0;

    Info->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer = FileInformation;
    Info->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress = NULL;

    if (RestartScan) {

        Info->Iopb->OperationFlags |= SL_RESTART_SCAN;
    }

    if (ReturnSingleEntry) {

        Info->Iopb->OperationFlags |= SL_RETURN_SINGLE_ENTRY;
    }

    //
    //  Perform the operation
    //

    FltPerformSynchronousIo( Info );

    Status = Info->IoStatus.Status;

    if (ARGUMENT_PRESENT(LengthReturned) &&
        NT_SUCCESS( Status )) {

        *LengthReturned = (ULONG) Info->IoStatus.Information;
    }

    FltFreeCallbackData( Info );

    return Status;
}

NTSTATUS
FLTAPI
NcCreateFileEx2Alternate (
    _In_            PFLT_FILTER Filter,
    _In_opt_        PFLT_INSTANCE Instance,
    _Out_           PHANDLE FileHandle,
    _Outptr_opt_ PFILE_OBJECT *FileObject,
    _In_            ACCESS_MASK DesiredAccess,
    _In_            POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_           PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_        PLARGE_INTEGER AllocationSize,
    _In_            ULONG FileAttributes,
    _In_            ULONG ShareAccess,
    _In_            ULONG CreateDisposition,
    _In_            ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_            ULONG EaLength,
    _In_            ULONG Flags,
    _In_opt_        PIO_DRIVER_CREATE_CONTEXT DriverContext
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  If we are here, FltCreateFileEx2 does not exist.  We
    //  cannot open files within the context of a transaction
    //  from here.  If Txf exists, so should FltCreateFileEx2.
    //

    UNREFERENCED_PARAMETER( DriverContext );
    FLT_ASSERT( DriverContext == NULL );

    //
    //  Zero out output parameters.
    //
    
    *FileHandle = INVALID_HANDLE_VALUE;
    
    if (ARGUMENT_PRESENT( FileObject )) {

        *FileObject = NULL;
    }

    if (NcCreateFileEx) {

        //
        //  If the system has FltCreateFileEx, we call that.
        //

        Status = NcCreateFileEx( Filter,
                                 Instance,
                                 FileHandle,
                                 FileObject,
                                 DesiredAccess,
                                 ObjectAttributes,
                                 IoStatusBlock,
                                 AllocationSize,
                                 FileAttributes,
                                 ShareAccess,
                                 CreateDisposition,
                                 CreateOptions,
                                 EaBuffer,
                                 EaLength,
                                 Flags );
    } else {

        //
        //  Attempt the open.
        //

        Status = FltCreateFile( Filter,
                                Instance,
                                FileHandle,
                                DesiredAccess,
                                ObjectAttributes,
                                IoStatusBlock,
                                AllocationSize,
                                FileAttributes,
                                ShareAccess,
                                CreateDisposition,
                                CreateOptions,
                                EaBuffer,
                                EaLength,
                                Flags );

        if(!NT_SUCCESS( Status )) {

            goto NcCreateFileEx2Cleanup;
        }

        FLT_ASSERT( *FileHandle != INVALID_HANDLE_VALUE && *FileHandle != NULL );

        if(ARGUMENT_PRESENT( FileObject )) {

            //
            //  If the user provided an output FileObject parameter,
            //  then we need to get a reference to the fileobject and return it.
            //

            Status = ObReferenceObjectByHandle( *FileHandle,
                                                DesiredAccess,
                                                *IoFileObjectType,
                                                KernelMode,
                                                FileObject,
                                                NULL );

            if(!NT_SUCCESS( Status )) {

                goto NcCreateFileEx2Cleanup;
            }
        }
    }

NcCreateFileEx2Cleanup:

    if (!NT_SUCCESS( Status )) {

        if (*FileHandle != INVALID_HANDLE_VALUE) {
    
            FltClose( *FileHandle );
        }

        if (ARGUMENT_PRESENT( FileObject )) {

            if (*FileObject != NULL) {

                ObDereferenceObject( *FileObject );
            }
        }
    }    

    return Status;
}    

//
//  Helper routines which manage importing the functions.
//

VOID 
NcCompatInit( )
{
    UNICODE_STRING FuncName;
    PAGED_CODE();

    //
    //  Default to NonPagedPoolNx for non paged pool allocations where supported.
    //
    
    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );

    //
    //  Cast from data pointer to function pointer.  This is
    //  somewhat unavoidable when calling MmGetSystemRoutineAddress/
    //  FltGetRoutineAddress etc.
    //

#pragma warning( push )
#pragma warning( disable: 4055 )

    RtlInitUnicodeString( &FuncName, L"IoReplaceFileObjectName" );

    NcReplaceFileObjectName = (NC_REPLACE_FILEOBJECT_NAME_TYPE)
        MmGetSystemRoutineAddress( &FuncName );

    if (NcReplaceFileObjectName == NULL) {

        NcReplaceFileObjectName = NcReplaceFileObjectNameAlternate;
    }

    NcQueryDirectoryFile = (NC_QUERY_DIRECTORY_FILE_TYPE)
        FltGetRoutineAddress( "FltQueryDirectoryFile" );

    if (NcQueryDirectoryFile == NULL) {

        NcQueryDirectoryFile = NcQueryDirectoryFileAlternate;
    }

    NcCreateFileEx2 = (NC_CREATE_FILE_EX2_TYPE)
        FltGetRoutineAddress( "FltCreateFileEx2" );

    if (NcCreateFileEx2 == NULL) {

        NcCreateFileEx = (NC_CREATE_FILE_EX_TYPE)
            FltGetRoutineAddress( "FltCreateFileEx" );
        NcCreateFileEx2 = NcCreateFileEx2Alternate;
    }

    NcGetNewSystemBufferAddress = (NC_GET_NEW_SYSTEM_BUFFER_ADDRESS)
        FltGetRoutineAddress( "FltGetNewSystemBufferAddress" );

#pragma warning( pop )
}


