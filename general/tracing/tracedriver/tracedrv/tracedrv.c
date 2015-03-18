/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    tracedrv.c   

Abstract:

    Sample kernel mode trace provider/driver.

--*/
#include <stdio.h>
#include <ntddk.h>
#include "drvioctl.h"
#include "tracedrv.h"
#include "tracedrv.tmh"      //  this is the file that will be auto generated


DRIVER_UNLOAD TracedrvDriverUnload;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH TracedrvDispatchOpenClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH TracedrvDispatchDeviceControl;

VOID
TraceEventLogger(
                IN PTRACEHANDLE pLoggerHandle
                );


DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           );

NTSTATUS
TracedrvDispatchOpenClose(
                    IN PDEVICE_OBJECT pDO,
                    IN PIRP Irp
                    );

NTSTATUS
TracedrvDispatchDeviceControl(
                             IN PDEVICE_OBJECT pDO,
                             IN PIRP Irp
                             );

VOID
TracedrvDriverUnload(
                    IN PDRIVER_OBJECT DriverObject
                    );


#ifdef ALLOC_PRAGMA
    #pragma alloc_text( INIT, DriverEntry )
    #pragma alloc_text( PAGE, TracedrvDispatchOpenClose )
    #pragma alloc_text( PAGE, TracedrvDispatchDeviceControl )
    #pragma alloc_text( PAGE, TracedrvDriverUnload )
#endif // ALLOC_PRAGMA


#define MAXEVENTS 3


NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:
    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path
               to driver-specific key in the registry

Return Value:

   STATUS_SUCCESS if successful
   STATUS_UNSUCCESSFUL  otherwise

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING deviceName;
    UNICODE_STRING linkName;
    PDEVICE_OBJECT pTracedrvDeviceObject;


    KdPrint(("TraceDrv: DriverEntry\n"));

    //
    // Create Dispatch Entry Points.  
    //
    DriverObject->DriverUnload = TracedrvDriverUnload;
    DriverObject->MajorFunction[ IRP_MJ_CREATE ] = TracedrvDispatchOpenClose;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE ] = TracedrvDispatchOpenClose;
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = TracedrvDispatchDeviceControl;    

    //
    // include this macro to support Win2K.
    //
    WPP_SYSTEMCONTROL(DriverObject);



    RtlInitUnicodeString( &deviceName, TRACEDRV_NT_DEVICE_NAME );

    //
    // Create the Device object
    //
    status = IoCreateDevice(
                           DriverObject,
                           0,
                           &deviceName,
                           FILE_DEVICE_UNKNOWN,
                           0,
                           FALSE,
                           &pTracedrvDeviceObject);

    if ( !NT_SUCCESS( status )) {
        return status;
    }

    RtlInitUnicodeString( &linkName, TRACEDRV_WIN32_DEVICE_NAME );
    status = IoCreateSymbolicLink( &linkName, &deviceName );

    if ( !NT_SUCCESS( status )) {
        IoDeleteDevice( pTracedrvDeviceObject );
        return status;
    }


    //
    // Choose a buffering mechanism
    //
    pTracedrvDeviceObject->Flags |= DO_BUFFERED_IO;


    //
    // This macro is required to initialize software tracing. 
    //
    // Win2K use the deviceobject as the first argument.
    //
    // XP and beyond does not require device object. First argument
    // is ignored. 
    // 
    WPP_INIT_TRACING(pTracedrvDeviceObject,RegistryPath);


    return STATUS_SUCCESS;
}

NTSTATUS
TracedrvDispatchOpenClose(
                    IN PDEVICE_OBJECT pDO,
                    IN PIRP Irp
                    )
/*++

Routine Description:

   Dispatch routine to handle Create/Close IRPs.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{

    UNREFERENCED_PARAMETER(pDO);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    PAGED_CODE();

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}


NTSTATUS
TracedrvDispatchDeviceControl(
                             IN PDEVICE_OBJECT pDO,
                             IN PIRP Irp
                             )
/*++

Routine Description:

   Dispatch routine to handle IOCTL IRPs.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack =  IoGetCurrentIrpStackLocation( Irp );
    ULONG ControlCode =  irpStack->Parameters.DeviceIoControl.IoControlCode;
    ULONG i=0;
    static ULONG ioctlCount = 0;
    MachineState CurrentState = Offline; 
    

    PAGED_CODE();
    UNREFERENCED_PARAMETER(pDO);

    Irp->IoStatus.Information = 
                            irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch ( ControlCode ) {
    case IOCTL_TRACEKMP_TRACE_EVENT:
        //
        // Every time we get this IOCTL, we also log a trace Message if
        // Trace flag one is enabled. This is used
        // to illustrate that the event can be caused by user-mode.
        //
        
        ioctlCount++;

        // 
        // Log a simple Message
        //

        DoTraceMessage(FLAG_ONE, "IOCTL = %d", ioctlCount);

        while (i++ < MAXEVENTS) {
            //
            // Trace events in a loop.
            //
            DoTraceMessage(FLAG_ONE,  "Hello, %d %s", i, "Hi" );

            if ( !(i%MAXEVENTS)){
                //
                // Trace if level >=2 and 2 bit set by -level 2 -flags 2 in tracelog
                // Uses the format string for the defined enum MachineState in the 
                // scanned header file
                //
                DoTraceLevelMessage(
                    TRACE_LEVEL_ERROR,          // ETW Level defined in evntrace.h
                    FLAG_TWO,                   // Flag defined in WPP_CONTROL_GUIDS
                    "Machine State :: %!state!",
                    CurrentState                // enum parameter 
                    );
            }
        }

        //
        // Set a fake error status to fire the TRACE_RETURN macro below
        //
        status = STATUS_DEVICE_POWERED_OFF;

        Irp->IoStatus.Information = 0;
        break;

        //
        // Not one we recognize. Error.
        //
    default:
        status = STATUS_INVALID_PARAMETER; 
        Irp->IoStatus.Information = 0;
        break;
    }

    //
    // Trace the return status using the TRACE_RETURN macro wich includes PRE/POST
    // macros. The value could be either the fake error or invalid parameter
    //
    TRACE_RETURN(status);

    if (status != STATUS_INVALID_PARAMETER) {
        //
        // Set the status back to success
        //
        status = STATUS_SUCCESS;
    }

    //
    // Get rid of this request
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
        
    return status;
}


VOID
TracedrvDriverUnload(
                    IN PDRIVER_OBJECT DriverObject
                    )
/*++

Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    PDEVICE_OBJECT pDevObj;
    UNICODE_STRING linkName;


    PAGED_CODE();

    KdPrint(("TraceDrv: Unloading \n"));

    //
    // Get pointer to Device object
    //    
    pDevObj = DriverObject->DeviceObject;

    // 
    // Cleanup using DeviceObject on Win2K. Make sure
    // this is same deviceobject that used for initializing.
    // On XP the Parameter is ignored
    WPP_CLEANUP(pDevObj);

    //
    // Form the Win32 symbolic link name.
    //
    RtlInitUnicodeString( &linkName, TRACEDRV_WIN32_DEVICE_NAME );

    //        
    // Remove symbolic link from Object
    // namespace...
    //
    IoDeleteSymbolicLink( &linkName );

    //
    // Unload the callbacks from the kernel to this driver
    //
    IoDeleteDevice( pDevObj );

}


