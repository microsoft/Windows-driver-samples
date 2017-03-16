/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    operations.c

Abstract:

    This is the CDO i/o operations module of the kernel mode filter driver implementing
    CDO sample


Environment:

    Kernel mode


--*/

#include "pch.h"


//
//  Assign text sections for each routine.
//

#ifdef  ALLOC_PRAGMA
    #pragma alloc_text( PAGE,     CdoCreateControlDeviceObject)
    #pragma alloc_text( PAGE,     CdoDeleteControlDeviceObject)
    #pragma alloc_text( PAGE,     CdoMajorFunction)
    #pragma alloc_text( PAGE,     CdoHandlePrivateOpen)
    #pragma alloc_text( PAGE,     CdoHandlePrivateCleanup)
    #pragma alloc_text( PAGE,     CdoHandlePrivateClose)
    #pragma alloc_text( PAGE,     CdoHandlePrivateFsControl)
    #pragma alloc_text( PAGE,     CdoFastIoCheckIfPossible)
    #pragma alloc_text( PAGE,     CdoFastIoRead)
    #pragma alloc_text( PAGE,     CdoFastIoWrite)
    #pragma alloc_text( PAGE,     CdoFastIoQueryBasicInfo)
    #pragma alloc_text( PAGE,     CdoFastIoQueryStandardInfo)
    #pragma alloc_text( PAGE,     CdoFastIoLock)
    #pragma alloc_text( PAGE,     CdoFastIoUnlockSingle)
    #pragma alloc_text( PAGE,     CdoFastIoUnlockAll)
    #pragma alloc_text( PAGE,     CdoFastIoUnlockAllByKey)
    #pragma alloc_text( PAGE,     CdoFastIoDeviceControl)
    #pragma alloc_text( PAGE,     CdoFastIoQueryNetworkOpenInfo)
    #pragma alloc_text( PAGE,     CdoFastIoMdlRead)
    #pragma alloc_text( NONPAGED, CdoFastIoMdlReadComplete)
    #pragma alloc_text( PAGE,     CdoFastIoPrepareMdlWrite)
    #pragma alloc_text( NONPAGED, CdoFastIoMdlWriteComplete)
    #pragma alloc_text( PAGE,     CdoFastIoReadCompressed)
    #pragma alloc_text( PAGE,     CdoFastIoWriteCompressed)
    #pragma alloc_text( NONPAGED, CdoFastIoMdlReadCompleteCompressed)
    #pragma alloc_text( NONPAGED, CdoFastIoMdlWriteCompleteCompressed)
    #pragma alloc_text( PAGE,     CdoFastIoQueryOpen)
    #pragma alloc_text( PAGE,     CdoHandlePrivateOpen )
    #pragma alloc_text( PAGE,     CdoHandlePrivateCleanup )
    #pragma alloc_text( PAGE,     CdoHandlePrivateClose )
    #pragma alloc_text( PAGE,     CdoHandlePrivateFsControl )

#endif


//
//  Fast IO dispatch routines
//

FAST_IO_DISPATCH CdoFastIoDispatch =
{
    sizeof(FAST_IO_DISPATCH),
    CdoFastIoCheckIfPossible,           //  CheckForFastIo
    CdoFastIoRead,                      //  FastIoRead
    CdoFastIoWrite,                     //  FastIoWrite
    CdoFastIoQueryBasicInfo,            //  FastIoQueryBasicInfo
    CdoFastIoQueryStandardInfo,         //  FastIoQueryStandardInfo
    CdoFastIoLock,                      //  FastIoLock
    CdoFastIoUnlockSingle,              //  FastIoUnlockSingle
    CdoFastIoUnlockAll,                 //  FastIoUnlockAll
    CdoFastIoUnlockAllByKey,            //  FastIoUnlockAllByKey
    CdoFastIoDeviceControl,             //  FastIoDeviceControl
    NULL,                               //  AcquireFileForNtCreateSection
    NULL,                               //  ReleaseFileForNtCreateSection
    NULL,                               //  FastIoDetachDevice
    CdoFastIoQueryNetworkOpenInfo,      //  FastIoQueryNetworkOpenInfo
    NULL,                               //  AcquireForModWrite
    CdoFastIoMdlRead,                   //  MdlRead
    CdoFastIoMdlReadComplete,           //  MdlReadComplete
    CdoFastIoPrepareMdlWrite,           //  PrepareMdlWrite
    CdoFastIoMdlWriteComplete,          //  MdlWriteComplete
    CdoFastIoReadCompressed,            //  FastIoReadCompressed
    CdoFastIoWriteCompressed,           //  FastIoWriteCompressed
    CdoFastIoMdlReadCompleteCompressed, //  MdlReadCompleteCompressed
    CdoFastIoMdlWriteCompleteCompressed, //  MdlWriteCompleteCompressed
    CdoFastIoQueryOpen,                 //  FastIoQueryOpen
    NULL,                               //  ReleaseForModWrite
    NULL,                               //  AcquireForCcFlush
    NULL,                               //  ReleaseForCcFlush
};



NTSTATUS
_Function_class_(DRIVER_INITIALIZE)
CdoCreateControlDeviceObject(
    _Inout_ PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine handles the IRPs that are directed to the control
    device object.

Arguments:

    DriverObject - driver object for this driver

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    UNICODE_STRING nameString;
    ULONG i;

    PAGED_CODE();

    //
    // Create our control device object
    //

    DebugTrace( DEBUG_TRACE_CDO_CREATE_DELETE,
                ("[Cdo]: Creating CDO ... \n") );

    RtlInitUnicodeString( &nameString, CONTROL_DEVICE_OBJECT_NAME );
    status = IoCreateDevice( DriverObject,
                             0,
                             &nameString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &Globals.FilterControlDeviceObject);

    if ( !NT_SUCCESS( status ) ) {

        DebugTrace( DEBUG_TRACE_CDO_CREATE_DELETE | DEBUG_TRACE_ERROR,
                    ("[Cdo]: Failure to create CDO. IoCreateDevice failed with status 0x%x. \n",
                      status) );
        return status;
    }

    //
    // Initialize the driver object with this driver's entry points.
    // Most are simply passed through to some other device driver.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

#pragma prefast(suppress:__WARNING_DISPATCH_MISMATCH __WARNING_DISPATCH_MISSING, "CdoMajorFunction is the dispatch routine for every major code, so no tag applies to it.")
        DriverObject->MajorFunction[i] = CdoMajorFunction;
    }

#pragma prefast(suppress:__WARNING_INACCESSIBLE_MEMBER, "The Cdo sample is allowed to set the FastIo Dispatch routine because he is setting up a Cdo.")
    DriverObject->FastIoDispatch = &CdoFastIoDispatch;

    DebugTrace( DEBUG_TRACE_CDO_CREATE_DELETE,
                ("[Cdo]: Creating CDO successful\n") );

    return STATUS_SUCCESS;
}



VOID
CdoDeleteControlDeviceObject(
    VOID
    )
/*++

Routine Description:

    This routine deletes the control device object.

Arguments:

    None

Return Value:

    None

--*/
{
    PAGED_CODE();

    //
    // Delete our control device object
    //

    DebugTrace( DEBUG_TRACE_CDO_CREATE_DELETE,
                ("[Cdo]: Deleting CDO ... \n") );

    IoDeleteDevice( Globals.FilterControlDeviceObject );

    DebugTrace( DEBUG_TRACE_CDO_CREATE_DELETE,
                ("[Cdo]: Deleting CDO successful\n") );

}


DRIVER_DISPATCH CdoMajorFunction;
NTSTATUS
CdoMajorFunction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
/*++

Routine Description:

    This routine handles the IRPs that are directed to the control
    device object.

Arguments:

    DeviceObject - control device object
    Irp          - the current Irp to process

Return Value:

    Returns STATUS_INVALID_DEVICE_REQUEST if the CDO doesn't support that request
    type, or the appropriate status otherwise.

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    FLT_ASSERT( IS_MY_CONTROL_DEVICE_OBJECT( DeviceObject ) );


    //
    //  default to success
    //

    status = STATUS_SUCCESS;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS,
                ("[Cdo]: CdoMajorFunction entry ( Irp = %p, irpSp->MajorFunction = 0x%x )\n",
                 Irp,
                 irpSp->MajorFunction) );

    switch (irpSp->MajorFunction) {

        //
        // IRP_MJ_CREATE is called to create a new HANDLE on CDO
        //

        case IRP_MJ_CREATE:
        {

            //
            //  Handle our private open
            //

            status = CdoHandlePrivateOpen(Irp);

            Irp->IoStatus.Status = status;

            if(NT_SUCCESS(status))
            {
                //
                //  If successful, return the file was opened
                //

                Irp->IoStatus.Information = FILE_OPENED;
            }
            else
            {
                Irp->IoStatus.Information = 0;
            }

            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            break;
        }

        //
        // IRP_MJ_CLOSE is called when all references are gone.
        //      Note:  this operation can not be failed.  It must succeed.
        //

        case IRP_MJ_CLOSE:
        {

            CdoHandlePrivateClose( Irp );

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            break;
        }

        //
        // IRP_MJ_DEVICE_CONTROL is how most user-mode api's drop into here
        //

        case IRP_MJ_FILE_SYSTEM_CONTROL:
        {
            ULONG Operation;
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            PVOID InputBuffer;
            PVOID OutputBuffer;

            Operation = irpSp->Parameters.FileSystemControl.FsControlCode;
            InputBufferLength = irpSp->Parameters.FileSystemControl.InputBufferLength;
            OutputBufferLength = irpSp->Parameters.FileSystemControl.OutputBufferLength;

            InputBuffer = Irp->AssociatedIrp.SystemBuffer;
            OutputBuffer = Irp->AssociatedIrp.SystemBuffer;

            //
            //  The caller will update the IO status block
            //

            status = CdoHandlePrivateFsControl (DeviceObject,
                                                Operation,
                                                InputBuffer,
                                                InputBufferLength,
                                                OutputBuffer,
                                                OutputBufferLength,
                                                &Irp->IoStatus,
                                                Irp );
                                                
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
                                                
            break;
        }

        //
        // IRP_MJ_CLEANUP is called when all handles are closed
        //      Note:  this operation can not be failed.  It must succeed.
        //

        case IRP_MJ_CLEANUP:
        {

            CdoHandlePrivateCleanup( Irp );

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            break;
        }

        default:
        {
            //
            // unsupported!
            //

            DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_ERROR,
                        ("[Cdo]: Unsupported Major Function 0x%x ( Irp = %p )\n",
                         irpSp->MajorFunction,
                         Irp) );

            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }


    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS,
                ("[Cdo]: CdoMajorFunction exit ( Irp = %p, irpSp->MajorFunction = 0x%x, status = 0x%x )\n",
                 Irp,
                 irpSp->MajorFunction,
                 status) );

    return status;


}


NTSTATUS
CdoHandlePrivateOpen(
    _In_ PIRP Irp
    )
/*++

Routine Description:

    This routine handles create IRPs that are directed to the control
    device object.

Arguments:

    Irp          - the current Irp to process

Return Value:

    Returns STATUS_DEVICE_ALREADY_ATTACHED if the CDO has already been opened
    Returns STATUS_SUCCESS otherwise

Note:

    This sample supports only one outstanding create on the CDO at a time

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( Irp );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateOpen entry ( Irp = %p )\n",
                 Irp) );

    CdoAcquireResourceExclusive( &Globals.Resource );

    if (FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_HANDLE ) ||
        FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_REF )) {

        //
        //  Sanity - if we have a handle open against this CDO
        //  we must have an outstanding reference as well
        //

        FLT_ASSERT( !FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_HANDLE ) ||
                    FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_REF ) );


        //
        // The CDO is already open - fail this open
        //

        status = STATUS_DEVICE_ALREADY_ATTACHED;

        DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Cdo]: CdoHandlePrivateOpen -> Device open failure. Device already opened. ( Irp = %p, Flags = 0x%x, status = 0x%x )\n",
                     Irp,
                     Globals.Flags,
                     status) );

    } else {

        //
        //  Flag that the CDO is opened so that we will fail future creates
        //  until the CDO is closed by the current caller
        //
        //
        //  If we suceed the create we are guaranteed to get a Cleanup (where we
        //  will reset GLOBAL_DATA_F_CDO_OPEN_HANDLE) and Close (where we will
        //  reset GLOBAL_DATA_F_CDO_OPEN_REF)
        //

        SetFlag( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_REF );
        SetFlag( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_HANDLE );

        status = STATUS_SUCCESS;

        DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Cdo]: CdoHandlePrivateOpen -> Device open successful. ( Irp = %p, Flags = 0x%x, status = 0x%x )\n",
                     Irp,
                     Globals.Flags,
                     status) );
    }


    //
    //  The filter may want to do additional processing here to set up the structures it
    //  needs to service this create request.
    //


    CdoReleaseResource( &Globals.Resource );


    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateOpen exit ( Irp = %p, status = 0x%x )\n",
                 Irp,
                 status) );


    return status;
}

NTSTATUS
CdoHandlePrivateCleanup(
    _In_ PIRP Irp
    )
/*++

Routine Description:

    This routine handles cleanup IRPs that are directed to the control
    device object.

Arguments:

    Irp          - the current Irp to process

Return Value:

    Returns STATUS_SUCCESS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( Irp );

    PAGED_CODE();


    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateCleanup entry ( Irp = %p )\n",
                 Irp) );


    CdoAcquireResourceExclusive( &Globals.Resource );

    //
    //  Sanity - the CDO must have a handle and a reference for us to get a cleanup on it
    //

    FLT_ASSERT( FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_REF ) &&
                FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_HANDLE) );


    //
    //  Reset the flag that indicates the CDO has a open handle
    //

    ClearFlag( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_HANDLE);

    status = STATUS_SUCCESS;

    //
    //  The filter may want to do additional processing here to cleanup up the structures it
    //  needed to service the handle that is being closed.
    //

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoHandlePrivateCleanup -> Device cleanup successful. ( Irp = %p, Flags = 0x%x, status = 0x%x )\n",
                 Irp,
                 Globals.Flags,
                 status) );


    CdoReleaseResource( &Globals.Resource );

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateCleanup exit ( Irp = %p, status = 0x%x )\n",
                 Irp,
                 status) );



    return status;
}

NTSTATUS
CdoHandlePrivateClose(
    _In_ PIRP Irp
    )
/*++

Routine Description:

    This routine handles close IRPs that are directed to the control
    device object.

Arguments:

    Irp          - the current Irp to process

Return Value:

    Returns STATUS_SUCCESS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( Irp );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateClose entry ( Irp = %p )\n",
                 Irp) );

    CdoAcquireResourceExclusive( &Globals.Resource );

    //
    //  Sanity - the connection must have a reference but have no handle open,
    //  for us to get a close on it
    //

    FLT_ASSERT( FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_REF ) &&
                !FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_HANDLE ));


    //
    //  Reset the flag that indicates the CDO is opened so that we will suceed
    //  future creates
    //

    ClearFlag( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_REF );


    //
    //  The filter may want to do additional processing here to cleanup up the structures it
    //  needed to service the user mode attachment that is being closed.
    //


    status = STATUS_SUCCESS;

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoHandlePrivateClose -> Device close successful. ( Irp = %p, Flags = 0x%x, status = 0x%x )\n",
                 Irp,
                 Globals.Flags,
                 status) );

    CdoReleaseResource( &Globals.Resource );


    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateClose exit ( Irp = %p, status = 0x%x )\n",
                 Irp,
                 status) );


    return status;

}

NTSTATUS
CdoHandlePrivateFsControl (
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_opt_ PIRP Irp
    )
/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered for the CDO.

Arguments:

    DeviceObject        - Pointer to the device object for this driver.
    IoControlCode       - Control code for this IOCTL
    InputBuffer         - Input buffer
    InputBufferLength   - Input buffer length
    OutputBuffer        - Output buffer
    OutputBufferLength  - Output buffer length
    IoStatus            - IO status block for this request
    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( IoControlCode );
    UNREFERENCED_PARAMETER( InputBuffer );
    UNREFERENCED_PARAMETER( InputBufferLength );
    UNREFERENCED_PARAMETER( OutputBuffer );
    UNREFERENCED_PARAMETER( OutputBufferLength );
    UNREFERENCED_PARAMETER( Irp );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateFsControl entry ( Irp = %p )\n"
                 "\tIoControlCode = 0x%x\n"
                 "\tInputBuffer = %p\n"
                 "\tInputBufferLength = 0x%x\n"
                 "\tOutputBuffer = %p\n"
                 "\tOutputBufferLength = 0x%x\n",
                 Irp,
                 IoControlCode,
                 InputBuffer,
                 InputBufferLength,
                 OutputBuffer,
                 OutputBufferLength) );

    CdoAcquireResourceShared( &Globals.Resource );

    //
    //  Sanity - there must atleast be a reference open for us to get a IOCTL on the CDO
    //

    FLT_ASSERT( FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_REF ) );


    if (!FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_HANDLE)) {

        //
        //  If there is no handle open to the CDO fail the operation
        //

        DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Cdo]: CdoHandlePrivateFsControl -> Failing IOCTL since no handle to CDO is open. ( Irp = %p, IoControlCode = 0x%x, Flags = 0x%x )\n",
                     Irp,
                     IoControlCode,
                     Globals.Flags) );

        status = STATUS_INVALID_DEVICE_STATE;
        CdoReleaseResource( &Globals.Resource );
        goto CdoHandlePrivateFsControlCleanup;
    }

    //
    //  Here the filter may perform any action that requires that
    //  the handle to the CDO still be open

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateFsControl -> Processing IOCTL while handle to CDO is definitely open. ( Irp = %p, IoControlCode = 0x%x )\n",
                 Irp,
                 IoControlCode) );

    CdoReleaseResource( &Globals.Resource );

    //
    //  Since the resource has been released the CDO may complete a cleanup before we
    //  do any of the following.
    //


    //
    //  Here the filter may perform any action that does not require that
    //  the handle to the CDO still be open. For example, the IOCTL may have
    //  been used to trigger off an asynchronous background task that will
    //  continue executing even after the handle has been closed
    //
    //  Note that the system will still maintain a reference to the CDO. So,
    //  the filter will not see a Close on the CDO until it finishes servicing
    //  IRP_MJ_FILE_SYSTEM_CONTROL
    //

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateFsControl -> Processing IOCTL while handle to CDO may not be open. ( Irp = %p, IoControlCode = 0x%x )\n",
                 Irp,
                 IoControlCode) );

    status = STATUS_SUCCESS;

CdoHandlePrivateFsControlCleanup:

    IoStatus->Status = status;
    IoStatus->Information = 0;

    DebugTrace( DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoHandlePrivateFsControl exit ( Irp = %p, IoControlCode = 0x%x, status = 0x%x )\n",
                 Irp,
                 IoControlCode,
                 status) );


    return status;
}



/////////////////////////////////////////////////////////////////////////////
//
//                      FastIO Handling routines
//
/////////////////////////////////////////////////////////////////////////////



BOOLEAN
CdoFastIoCheckIfPossible (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ BOOLEAN CheckForReadOperation,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for checking to see
    whether fast I/O is possible for this file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be operated on.

    FileOffset - Byte offset in the file for the operation.

    Length - Length of the operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    CheckForReadOperation - Indicates whether the caller is checking for a
        read (TRUE) or a write operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(CheckForReadOperation);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoCheckIfPossible -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}


BOOLEAN
CdoFastIoRead (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading from a
    file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be read.

    FileOffset - Byte offset in the file of the read.

    Length - Length of the read operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer to receive the data read.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoRead -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}



BOOLEAN
CdoFastIoWrite (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_reads_bytes_(Length) PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing to a
    file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be written.

    FileOffset - Byte offset in the file of the write operation.

    Length - Length of the write operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer that contains the data to be
        written.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoWrite -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}

//  This annotation tells the static analyzer that IoStatus->Status is where to check
//  whether this routine succeeded or not, not the BOOLEAN return value.
_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoQueryBasicInfo (
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_BASIC_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying basic
    information about the file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoQueryBasicInfo -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}


//  This annotation tells the static analyzer that IoStatus->Status is where to check
//  whether this routine succeeded or not, not the BOOLEAN return value.
_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoQueryStandardInfo (
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_STANDARD_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying standard
    information about the file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoQueryStandardInfo -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}


BOOLEAN
CdoFastIoLock (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ PEPROCESS ProcessId,
    _In_ ULONG Key,
    _In_ BOOLEAN FailImmediately,
    _In_ BOOLEAN ExclusiveLock,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for locking a byte
    range within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be locked.

    FileOffset - Starting byte offset from the base of the file to be locked.

    Length - Length of the byte range to be locked.

    ProcessId - ID of the process requesting the file lock.

    Key - Lock key to associate with the file lock.

    FailImmediately - Indicates whether or not the lock request is to fail
        if it cannot be immediately be granted.

    ExclusiveLock - Indicates whether the lock to be taken is exclusive (TRUE)
        or shared.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(FailImmediately);
    UNREFERENCED_PARAMETER(ExclusiveLock);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoLock -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}


BOOLEAN
CdoFastIoUnlockSingle (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ PEPROCESS ProcessId,
    _In_ ULONG Key,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking a byte
    range within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    FileOffset - Starting byte offset from the base of the file to be
        unlocked.

    Length - Length of the byte range to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the file lock.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoUnlockSingle -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}



BOOLEAN
CdoFastIoUnlockAll (
    _In_ PFILE_OBJECT FileObject,
    _In_ PEPROCESS ProcessId,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoUnlockAll -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}


BOOLEAN
CdoFastIoUnlockAllByKey (
    _In_ PFILE_OBJECT FileObject,
    _In_ PVOID ProcessId,
    _In_ ULONG Key,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file based on a specified key.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the locks on the file to be released.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoUnlockAllByKey -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}



BOOLEAN
CdoFastIoDeviceControl (
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ ULONG IoControlCode,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for device I/O control
    operations on a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object representing the device to be
        serviced.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    InputBuffer - Optional pointer to a buffer to be passed into the driver.

    InputBufferLength - Length of the optional InputBuffer, if one was
        specified.

    OutputBuffer - Optional pointer to a buffer to receive data from the
        driver.

    OutputBufferLength - Length of the optional OutputBuffer, if one was
        specified.

    IoControlCode - I/O control code indicating the operation to be performed
        on the device.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Wait);

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoFastIoDeviceControl Entry ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    //
    //  The caller will update the IO status block
    //

    CdoHandlePrivateFsControl ( DeviceObject,
                                IoControlCode,
                                InputBuffer,
                                InputBufferLength,
                                OutputBuffer,
                                OutputBufferLength,
                                IoStatus,
                                NULL );

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS,
                ("[Cdo]: CdoFastIoDeviceControl Exit ( FileObject = %p, DeviceObject = %p, Status = 0x%x )\n",
                 FileObject,
                 DeviceObject,
                 IoStatus->Status) );

    return TRUE;
}


//  This annotation tells the static analyzer that IoStatus->Status is where to check
//  whether this routine succeeded or not, not the BOOLEAN return value.
_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoQueryNetworkOpenInfo (
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying network
    information about a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller can handle the file system
        having to wait and tie up the current thread.

    Buffer - Pointer to a buffer to receive the network information about the
        file.

    IoStatus - Pointer to a variable to receive the final status of the query
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoQueryNetworkOpenInfo -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}


//  This annotation tells the static analyzer that IoStatus->Status is where to check
//  whether this routine succeeded or not, not the BOOLEAN return value.
_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoMdlRead (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Outptr_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading a file
    using MDLs as buffers.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that is to be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoMdlRead -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}

BOOLEAN
CdoFastIoMdlReadComplete (
    _In_ PFILE_OBJECT FileObject,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the MdlRead function is supported by the underlying file system, and
    therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL read upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, return not supported
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoMdlReadComplete -> Unsupported as FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    return FALSE;
}


//  This annotation tells the static analyzer that IoStatus->Status is where to check
//  whether this routine succeeded or not, not the BOOLEAN return value.
_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoPrepareMdlWrite (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Outptr_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for preparing for an
    MDL write operation.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoPrepareMdlWrite -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}




BOOLEAN
CdoFastIoMdlWriteComplete (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL write operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the PrepareMdlWrite function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL write upon.

    FileOffset - Supplies the file offset at which the write took place.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/
{
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, return not supported
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoMdlWriteComplete -> Unsupported as FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );


    return FALSE;
}


/*********************************************************************************
        UNIMPLEMENTED FAST IO ROUTINES

        The following four Fast IO routines are for compression on the wire
        which is not yet implemented in NT.

        NOTE:  It is highly recommended that you include these routines (which
               do a pass-through call) so your filter will not need to be
               modified in the future when this functionality is implemented in
               the OS.

        FastIoReadCompressed, FastIoWriteCompressed,
        FastIoMdlReadCompleteCompressed, FastIoMdlWriteCompleteCompressed
**********************************************************************************/



//  This annotation tells the static analyzer that IoStatus->Status is where to check
//  whether this routine succeeded or not, not the BOOLEAN return value.
_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoReadCompressed (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _Outptr_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _Out_writes_bytes_(CompressedDataInfoLength) struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    _In_ ULONG CompressedDataInfoLength,
    _In_ PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading compressed
    data from a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to a buffer to receive the compressed data read.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    CompressedDataInfo - A buffer to receive the description of the compressed
        data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(CompressedDataInfo);
    UNREFERENCED_PARAMETER(CompressedDataInfoLength);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoReadCompressed -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}


//  This annotation tells the static analyzer that IoStatus->Status is where to check
//  whether this routine succeeded or not, not the BOOLEAN return value.
_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoWriteCompressed (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _In_reads_bytes_(Length) PVOID Buffer,
    _Outptr_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_reads_bytes_(CompressedDataInfoLength) struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    _In_ ULONG CompressedDataInfoLength,
    _In_ PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing compressed
    data to a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to the buffer containing the data to be written.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    CompressedDataInfo - A buffer to containing the description of the
        compressed data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockKey);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(CompressedDataInfo);
    UNREFERENCED_PARAMETER(CompressedDataInfoLength);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoWriteCompressed -> Unsupported FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoStatus->Information = 0;

    return TRUE;
}




BOOLEAN
CdoFastIoMdlReadCompleteCompressed (
    _In_ PFILE_OBJECT FileObject,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read compressed operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the read compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed read
        upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/
{
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, return not supported
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoMdlReadCompleteCompressed -> Unsupported as FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    return FALSE;
}



BOOLEAN
CdoFastIoMdlWriteCompleteCompressed (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing a
    write compressed operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the write compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed write
        upon.

    FileOffset - Supplies the file offset at which the file write operation
        began.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/
{
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(MdlChain);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, return not supported
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoMdlWriteCompleteCompressed -> Unsupported as FastIO call ( FileObject = %p, DeviceObject = %p )\n",
                 FileObject,
                 DeviceObject) );

    return FALSE;
}


//  This annotation tells the static analyzer that IoStatus->Status is where to check
//  whether this routine succeeded or not, not the BOOLEAN return value.
_Success_(Irp->IoStatus.Status == 0)
BOOLEAN
CdoFastIoQueryOpen (
    _In_ PIRP Irp,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    _In_ PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for opening a file
    and returning network information for it.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    Irp - Pointer to a create IRP that represents this open operation.  It is
        to be used by the file system for common open/create code, but not
        actually completed.

    NetworkInformation - A buffer to receive the information required by the
        network about the file being opened.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/
{
    PAGED_CODE();
    FLT_ASSERT(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    UNREFERENCED_PARAMETER(NetworkInformation);
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  This is our CDO, fail the operation
    //

    DebugTrace( DEBUG_TRACE_CDO_ALL_OPERATIONS | DEBUG_TRACE_CDO_FASTIO_OPERATIONS | DEBUG_TRACE_ERROR,
                ("[Cdo]: CdoFastIoQueryOpen -> Unsupported FastIO call ( Irp = %p, DeviceObject = %p )\n",
                 Irp,
                 DeviceObject) );

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;

    return TRUE;
}



