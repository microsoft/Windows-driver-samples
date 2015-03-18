/*++

Copyright (c) 1990-98  Microsoft Corporation All Rights Reserved

Module Name:

    sioctl.c

Abstract:

    Purpose of this driver is to demonstrate how the four different types
    of IOCTLs can be used, and how the I/O manager handles the user I/O
    buffers in each case. This sample also helps to understand the usage of
    some of the memory manager functions.

Environment:

    Kernel mode only.

--*/


//
// Include files.
//

#include <ntddk.h>          // various NT definitions
#include <string.h>

#include "sioctl.h"

#define NT_DEVICE_NAME      L"\\Device\\SIOCTL"
#define DOS_DEVICE_NAME     L"\\DosDevices\\IoctlTest"

#if DBG
#define SIOCTL_KDPRINT(_x_) \
                DbgPrint("SIOCTL.SYS: ");\
                DbgPrint _x_;

#else
#define SIOCTL_KDPRINT(_x_)
#endif

//
// Device driver routine declarations.
//

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH SioctlCreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH SioctlDeviceControl;

DRIVER_UNLOAD SioctlUnloadDriver;

VOID
PrintIrpInfo(
    PIRP Irp
    );
VOID
PrintChars(
    _In_reads_(CountChars) PCHAR BufferAddress,
    _In_ size_t CountChars
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, SioctlCreateClose)
#pragma alloc_text( PAGE, SioctlDeviceControl)
#pragma alloc_text( PAGE, SioctlUnloadDriver)
#pragma alloc_text( PAGE, PrintIrpInfo)
#pragma alloc_text( PAGE, PrintChars)
#endif // ALLOC_PRAGMA


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT   DriverObject,
    _In_ PUNICODE_STRING      RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It creates the device object, fills in the dispatch entry points and
    completes the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if initialized; an error otherwise.

--*/

{
    NTSTATUS        ntStatus;
    UNICODE_STRING  ntUnicodeString;    // NT Device Name "\Device\SIOCTL"
    UNICODE_STRING  ntWin32NameString;    // Win32 Name "\DosDevices\IoctlTest"
    PDEVICE_OBJECT  deviceObject = NULL;    // ptr to device object

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString( &ntUnicodeString, NT_DEVICE_NAME );

    ntStatus = IoCreateDevice(
        DriverObject,                   // Our Driver Object
        0,                              // We don't use a device extension
        &ntUnicodeString,               // Device name "\Device\SIOCTL"
        FILE_DEVICE_UNKNOWN,            // Device type
        FILE_DEVICE_SECURE_OPEN,     // Device characteristics
        FALSE,                          // Not an exclusive device
        &deviceObject );                // Returned ptr to Device Object

    if ( !NT_SUCCESS( ntStatus ) )
    {
        SIOCTL_KDPRINT(("Couldn't create the device object\n"));
        return ntStatus;
    }

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SioctlCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SioctlCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SioctlDeviceControl;
    DriverObject->DriverUnload = SioctlUnloadDriver;

    //
    // Initialize a Unicode String containing the Win32 name
    // for our device.
    //

    RtlInitUnicodeString( &ntWin32NameString, DOS_DEVICE_NAME );

    //
    // Create a symbolic link between our device name  and the Win32 name
    //

    ntStatus = IoCreateSymbolicLink(
                        &ntWin32NameString, &ntUnicodeString );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        //
        // Delete everything that this routine has allocated.
        //
        SIOCTL_KDPRINT(("Couldn't create symbolic link\n"));
        IoDeleteDevice( deviceObject );
    }


    return ntStatus;
}


NTSTATUS
SioctlCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when the SIOCTL is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

VOID
SioctlUnloadDriver(
    _In_ PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver.

Return Value:

    None
--*/

{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    PAGED_CODE();

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );


    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //

    IoDeleteSymbolicLink( &uniWin32NameString );

    if ( deviceObject != NULL )
    {
        IoDeleteDevice( deviceObject );
    }



}

NTSTATUS
SioctlDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

Arguments:

    DeviceObject - a pointer to the object that represents the device
        that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    PIO_STACK_LOCATION  irpSp;// Pointer to current stack location
    NTSTATUS            ntStatus = STATUS_SUCCESS;// Assume success
    ULONG               inBufLength; // Input buffer length
    ULONG               outBufLength; // Output buffer length
    PCHAR               inBuf, outBuf; // pointer to Input and output buffer
    PCHAR               data = "This String is from Device Driver !!!";
    size_t              datalen = strlen(data)+1;//Length of data including null
    PMDL                mdl = NULL;
    PCHAR               buffer = NULL;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (!inBufLength || !outBufLength)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

    //
    // Determine which I/O control code was specified.
    //

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode )
    {
    case IOCTL_SIOCTL_METHOD_BUFFERED:

        //
        // In this method the I/O manager allocates a buffer large enough to
        // to accommodate larger of the user input buffer and output buffer,
        // assigns the address to Irp->AssociatedIrp.SystemBuffer, and
        // copies the content of the user input buffer into this SystemBuffer
        //

        SIOCTL_KDPRINT(("Called IOCTL_SIOCTL_METHOD_BUFFERED\n"));
        PrintIrpInfo(Irp);

        //
        // Input buffer and output buffer is same in this case, read the
        // content of the buffer before writing to it
        //

        inBuf = Irp->AssociatedIrp.SystemBuffer;
        outBuf = Irp->AssociatedIrp.SystemBuffer;

        //
        // Read the data from the buffer
        //

        SIOCTL_KDPRINT(("\tData from User :"));
        //
        // We are using the following function to print characters instead
        // DebugPrint with %s format because we string we get may or
        // may not be null terminated.
        //
        PrintChars(inBuf, inBufLength);

        //
        // Write to the buffer over-writes the input buffer content
        //

        RtlCopyBytes(outBuf, data, outBufLength);

        SIOCTL_KDPRINT(("\tData to User : "));
        PrintChars(outBuf, datalen  );

        //
        // Assign the length of the data copied to IoStatus.Information
        // of the Irp and complete the Irp.
        //

        Irp->IoStatus.Information = (outBufLength<datalen?outBufLength:datalen);

        //
        // When the Irp is completed the content of the SystemBuffer
        // is copied to the User output buffer and the SystemBuffer is
        // is freed.
        //

       break;

    case IOCTL_SIOCTL_METHOD_NEITHER:

        //
        // In this type of transfer the I/O manager assigns the user input
        // to Type3InputBuffer and the output buffer to UserBuffer of the Irp.
        // The I/O manager doesn't copy or map the buffers to the kernel
        // buffers. Nor does it perform any validation of user buffer's address
        // range.
        //


        SIOCTL_KDPRINT(("Called IOCTL_SIOCTL_METHOD_NEITHER\n"));

        PrintIrpInfo(Irp);

        //
        // A driver may access these buffers directly if it is a highest level
        // driver whose Dispatch routine runs in the context
        // of the thread that made this request. The driver should always
        // check the validity of the user buffer's address range and check whether
        // the appropriate read or write access is permitted on the buffer.
        // It must also wrap its accesses to the buffer's address range within
        // an exception handler in case another user thread deallocates the buffer
        // or attempts to change the access rights for the buffer while the driver
        // is accessing memory.
        //

        inBuf = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        outBuf =  Irp->UserBuffer;

        //
        // Access the buffers directly if only if you are running in the
        // context of the calling process. Only top level drivers are
        // guaranteed to have the context of process that made the request.
        //

        try {
            //
            // Before accessing user buffer, you must probe for read/write
            // to make sure the buffer is indeed an userbuffer with proper access
            // rights and length. ProbeForRead/Write will raise an exception if it's otherwise.
            //
            ProbeForRead( inBuf, inBufLength, sizeof( UCHAR ) );

            //
            // Since the buffer access rights can be changed or buffer can be freed
            // anytime by another thread of the same process, you must always access
            // it within an exception handler.
            //

            SIOCTL_KDPRINT(("\tData from User :"));
            PrintChars(inBuf, inBufLength);

        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {

            ntStatus = GetExceptionCode();
            SIOCTL_KDPRINT((
                "Exception while accessing inBuf 0X%08X in METHOD_NEITHER\n",
                            ntStatus));
            break;
        }


        //
        // If you are accessing these buffers in an arbitrary thread context,
        // say in your DPC or ISR, if you are using it for DMA, or passing these buffers to the
        // next level driver, you should map them in the system process address space.
        // First allocate an MDL large enough to describe the buffer
        // and initilize it. Please note that on a x86 system, the maximum size of a buffer
        // that an MDL can describe is 65508 KB.
        //

        mdl = IoAllocateMdl(inBuf, inBufLength,  FALSE, TRUE, NULL);
        if (!mdl)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        try
        {

            //
            // Probe and lock the pages of this buffer in physical memory.
            // You can specify IoReadAccess, IoWriteAccess or IoModifyAccess
            // Always perform this operation in a try except block.
            //  MmProbeAndLockPages will raise an exception if it fails.
            //
            MmProbeAndLockPages(mdl, UserMode, IoReadAccess);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {

            ntStatus = GetExceptionCode();
            SIOCTL_KDPRINT((
                "Exception while locking inBuf 0X%08X in METHOD_NEITHER\n",
                    ntStatus));
            IoFreeMdl(mdl);
            break;
        }

        //
        // Map the physical pages described by the MDL into system space.
        // Note: double mapping the buffer this way causes lot of
        // system overhead for large size buffers.
        //

        buffer = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority );

        if (!buffer) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                MmUnlockPages(mdl);
                IoFreeMdl(mdl);
                break;
        }

        //
        // Now you can safely read the data from the buffer.
        //
        SIOCTL_KDPRINT(("\tData from User (SystemAddress) : "));
        PrintChars(buffer, inBufLength);

        //
        // Once the read is over unmap and unlock the pages.
        //

        MmUnlockPages(mdl);
        IoFreeMdl(mdl);

        //
        // The same steps can be followed to access the output buffer.
        //

        mdl = IoAllocateMdl(outBuf, outBufLength,  FALSE, TRUE, NULL);
        if (!mdl)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }


        try {
            //
            // Probe and lock the pages of this buffer in physical memory.
            // You can specify IoReadAccess, IoWriteAccess or IoModifyAccess.
            //

            MmProbeAndLockPages(mdl, UserMode, IoWriteAccess);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {

            ntStatus = GetExceptionCode();
            SIOCTL_KDPRINT((
                "Exception while locking outBuf 0X%08X in METHOD_NEITHER\n",
                    ntStatus));
            IoFreeMdl(mdl);
            break;
        }


        buffer = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority );

        if (!buffer) {
            MmUnlockPages(mdl);
            IoFreeMdl(mdl);
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        //
        // Write to the buffer
        //

        RtlCopyBytes(buffer, data, outBufLength);

        SIOCTL_KDPRINT(("\tData to User : %s\n", buffer));
        PrintChars(buffer, datalen);

        MmUnlockPages(mdl);

        //
        // Free the allocated MDL
        //

        IoFreeMdl(mdl);

        //
        // Assign the length of the data copied to IoStatus.Information
        // of the Irp and complete the Irp.
        //

        Irp->IoStatus.Information = (outBufLength<datalen?outBufLength:datalen);

        break;

    case IOCTL_SIOCTL_METHOD_IN_DIRECT:

        //
        // In this type of transfer,  the I/O manager allocates a system buffer
        // large enough to accommodatethe User input buffer, sets the buffer address
        // in Irp->AssociatedIrp.SystemBuffer and copies the content of user input buffer
        // into the SystemBuffer. For the user output buffer, the  I/O manager
        // probes to see whether the virtual address is readable in the callers
        // access mode, locks the pages in memory and passes the pointer to
        // MDL describing the buffer in Irp->MdlAddress.
        //

        SIOCTL_KDPRINT(("Called IOCTL_SIOCTL_METHOD_IN_DIRECT\n"));

        PrintIrpInfo(Irp);

        inBuf = Irp->AssociatedIrp.SystemBuffer;

        SIOCTL_KDPRINT(("\tData from User in InputBuffer: "));
        PrintChars(inBuf, inBufLength);

        //
        // To access the output buffer, just get the system address
        // for the buffer. For this method, this buffer is intended for transfering data
        // from the application to the driver.
        //

        buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

        if (!buffer) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        SIOCTL_KDPRINT(("\tData from User in OutputBuffer: "));
        PrintChars(buffer, outBufLength);

        //
        // Return total bytes read from the output buffer.
        // Note OutBufLength = MmGetMdlByteCount(Irp->MdlAddress)
        //

        Irp->IoStatus.Information = MmGetMdlByteCount(Irp->MdlAddress);

        //
        // NOTE: Changes made to the  SystemBuffer are not copied
        // to the user input buffer by the I/O manager
        //

      break;

    case IOCTL_SIOCTL_METHOD_OUT_DIRECT:

        //
        // In this type of transfer, the I/O manager allocates a system buffer
        // large enough to accommodate the User input buffer, sets the buffer address
        // in Irp->AssociatedIrp.SystemBuffer and copies the content of user input buffer
        // into the SystemBuffer. For the output buffer, the I/O manager
        // probes to see whether the virtual address is writable in the callers
        // access mode, locks the pages in memory and passes the pointer to MDL
        // describing the buffer in Irp->MdlAddress.
        //


        SIOCTL_KDPRINT(("Called IOCTL_SIOCTL_METHOD_OUT_DIRECT\n"));

        PrintIrpInfo(Irp);


        inBuf = Irp->AssociatedIrp.SystemBuffer;

        SIOCTL_KDPRINT(("\tData from User : "));
        PrintChars(inBuf, inBufLength);

        //
        // To access the output buffer, just get the system address
        // for the buffer. For this method, this buffer is intended for transfering data
        // from the driver to the application.
        //

        buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

        if (!buffer) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Write data to be sent to the user in this buffer
        //

        RtlCopyBytes(buffer, data, outBufLength);

        SIOCTL_KDPRINT(("\tData to User : "));
        PrintChars(buffer, datalen);

        Irp->IoStatus.Information = (outBufLength<datalen?outBufLength:datalen);

        //
        // NOTE: Changes made to the  SystemBuffer are not copied
        // to the user input buffer by the I/O manager
        //

        break;

    default:

        //
        // The specified I/O control code is unrecognized by this driver.
        //

        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        SIOCTL_KDPRINT(("ERROR: unrecognized IOCTL %x\n",
            irpSp->Parameters.DeviceIoControl.IoControlCode));
        break;
    }

End:
    //
    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    //

    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return ntStatus;
}

VOID
PrintIrpInfo(
    PIRP Irp)
{
    PIO_STACK_LOCATION  irpSp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    SIOCTL_KDPRINT(("\tIrp->AssociatedIrp.SystemBuffer = 0x%p\n",
        Irp->AssociatedIrp.SystemBuffer));
    SIOCTL_KDPRINT(("\tIrp->UserBuffer = 0x%p\n", Irp->UserBuffer));
    SIOCTL_KDPRINT(("\tirpSp->Parameters.DeviceIoControl.Type3InputBuffer = 0x%p\n",
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer));
    SIOCTL_KDPRINT(("\tirpSp->Parameters.DeviceIoControl.InputBufferLength = %d\n",
        irpSp->Parameters.DeviceIoControl.InputBufferLength));
    SIOCTL_KDPRINT(("\tirpSp->Parameters.DeviceIoControl.OutputBufferLength = %d\n",
        irpSp->Parameters.DeviceIoControl.OutputBufferLength ));
    return;
}

VOID
PrintChars(
    _In_reads_(CountChars) PCHAR BufferAddress,
    _In_ size_t CountChars
    )
{
    PAGED_CODE();

    if (CountChars) {

        while (CountChars--) {

            if (*BufferAddress > 31
                 && *BufferAddress != 127) {

                KdPrint (( "%c", *BufferAddress) );

            } else {

                KdPrint(( ".") );

            }
            BufferAddress++;
        }
        KdPrint (("\n"));
    }
    return;
}


