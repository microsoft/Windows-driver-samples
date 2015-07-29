#include <pch.h>

/*
** Driver TODO:
**
** WDF calls this callback when a file handle is opened to the driver.  Your implementation may require additional setup (such as creating a
** file-handle-based context structure).  PosCxOpen must be called during this callback.
*/
VOID EvtDeviceFileCreate(_In_ WDFDEVICE Device, _In_ WDFREQUEST Request, _In_ WDFFILEOBJECT FileObject)
{
    NTSTATUS status = PosCxOpen(Device, FileObject, MSR_INTERFACE_TAG);

    if (!NT_SUCCESS(status))
    {
        // This should only fail in rare cases, but the failure will prevent all PosCx functions from performing correctly
    }

    WdfRequestComplete(Request, status);
}

/*
** Driver TODO:
**
** WDF calls this callback when a file handle to the driver is closed.  Your implementation may require additional cleanup, but 
** PosCxClose must be called during this callback.
*/
VOID EvtFileClose(_In_ WDFFILEOBJECT FileObject)
{
    WDFDEVICE device = WdfFileObjectGetDevice(FileObject);
    
    NTSTATUS status = PosCxClose(device, FileObject);

    if (!NT_SUCCESS(status))
    {
        // This will only fail if PosCxInit wasn't called successfully in EvtDriverDeviceAdd, or if PosCxOpen failed in EvtDeviceFileCreate
    }
}