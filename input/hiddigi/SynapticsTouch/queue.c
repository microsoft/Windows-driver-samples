/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        queue.c

    Abstract:

        Contains WDF queue related code, namely the top-level queue
        handler for all device IOCTL requests.

    Environment:

        Kernel mode

    Revision History:

--*/

#include "internal.h"
#include "controller.h"
#include "queue.h"
#include "hid.h"
#include "idle.h"
#include "queue.tmh"

VOID
OnInternalDeviceControl(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t OutputBufferLength,
    IN size_t InputBufferLength,
    IN ULONG IoControlCode
    )
/*++

Routine Description:

    This event is called when the framework receives 
    IRP_MJ_INTERNAL DEVICE_CONTROL requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                         if an output buffer is available.

    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.

Return Value:

    None, status is indicated when completing the request

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION devContext;
    WDFDEVICE device;
    BOOLEAN requestPending;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    
    device = WdfIoQueueGetDevice(Queue);
    devContext = GetDeviceContext(device);
    requestPending = FALSE;

    //
    // Please note that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl. So depending on the ioctl code, we will either
    // use retreive function or escape to WDM to get the UserBuffer.
    //

    switch(IoControlCode) {

    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        //
        // Retrieves master HID descriptor 
        //

        status = TchGetHidDescriptor(device, Request);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        //
        // Retrieves device attributes in a HID_DEVICE_ATTRIBUTES structure
        //
        
        status = TchGetDeviceAttributes(Request);
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        //
        // Obtains the report descriptor for the HID device
        //
        
        status = TchGetReportDescriptor(device, Request);
        break;

    case IOCTL_HID_GET_STRING:
        //
        // Obtains strings associated with the HID device
        //
        
        status = TchGetString(device, Request);
        break;

    case IOCTL_HID_READ_REPORT:
        //
        // Dangling read requests for passing up touch data
        //
        
        status = TchReadReport(device, Request, &requestPending);
        break;
    
    case IOCTL_HID_SET_FEATURE:
        //
        // This sends a HID class feature report to a top-level collection of
        // a HID class device.
        //
        
        status = TchSetFeatureReport(device, Request);
        break;
    
    case IOCTL_HID_GET_FEATURE:
        //
        // Returns a feature report associated with a top-level collection
        //
        
        status = TchGetFeatureReport(device, Request);
        break;

    case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:    
        //
        // Hidclass sends this IOCTL to notify miniports that it wants
        // them to go into idle
        //

        status = TchProcessIdleRequest(device, Request, &requestPending);
        break;

    case IOCTL_HID_WRITE_REPORT:
        //
        // Transmits a class driver-supplied report to the device.
        //    
    case IOCTL_HID_ACTIVATE_DEVICE:
        //
        // Makes the device ready for I/O operations.
        //
    case IOCTL_HID_DEACTIVATE_DEVICE:
        //
        // Causes the device to cease operations and terminate all outstanding
        // I/O requests.
        //

    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (!requestPending)
    {
        WdfRequestComplete(Request, status);
    }
    
    return;
}
