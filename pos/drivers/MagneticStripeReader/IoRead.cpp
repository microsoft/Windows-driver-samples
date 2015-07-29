#include <pch.h>

/*
** Driver TODO: Add logic to EvtIoRead to handle read requests from applications that don't use the Windows.Devices.PointOfService APIs.
**
** This is the callback for the IO queue that handles file read requests.  In the POS magnetic
** stripe reader model, the application will always queue a read request in order to receive events
** such as the data received event, or the release-claim requested event.
**
** Note that apps that are developed against the Windows.Devices.PointOfService APIs will always
** expect event data to be returned by read requests.  It is up to the driver to determine the
** behavior of ReadFile when the driver is opened by other types of applications.
*/
VOID EvtIoRead(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
    NTSTATUS status;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    WDFFILEOBJECT fileObject = WdfRequestGetFileObject(Request);

    UNREFERENCED_PARAMETER(Length);

    // Check the flag that may have been set by PosCxMarkPosApp in Ioctl.cpp.
    if (!PosCxIsPosApp(device, fileObject))
    {
        // An application has opened a handle to this device without using the Windows.Devices.PointOfService APIs.
        // You may change this to handle the read request differently.

        // In this example, just complete the read request with a failure.
        WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
    }
    else
    {
        // If this returns success, it has taken ownership of Request
        status = PosCxGetPendingEvent(device, Request);

        if (!NT_SUCCESS(status))
        {
            WdfRequestComplete(Request, status);
        }
    }
}