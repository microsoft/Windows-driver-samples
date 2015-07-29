#include <pch.h>

/*
** Driver TODO:  Add code to EvtDeviceOwnershipChange to reset the device state to a default.
**
** PosCx calls this callback to signal that the ownership of the device has transitioned from one file handle
** to another.  When this happens, app developers expect that the settings for the device are restored to a
** "default" state, so the driver should do what is necessary to satisfy that expectation here.
**
*/
VOID EvtDeviceOwnershipChange(_In_ WDFDEVICE Device, _In_opt_ WDFFILEOBJECT OldOwnerFileObj, _In_opt_ WDFFILEOBJECT NewOwnerFileObj)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(OldOwnerFileObj);
    UNREFERENCED_PARAMETER(NewOwnerFileObj);

    // This function signals that ownership has transitioned from one file handle to another (typically from one app to another).
    // As a result, the driver is expected to reset all settings to a default state.
}

/*
** Driver TODO:
**
** This part of the sample demonstrates how the driver will return data to the runtime.  How this method would be called depends
** on the driver implementation.
*/
VOID EvtOnMsrScanDataRetrieved(_In_ WDFDEVICE Device)
{
    // Events that the runtime can handle for a magnetic stripe reader device are:
    //  PosEventType::MagneticStripeReaderDataReceived -- the standard event with MSR data
    //  PosEventType::MagneticStripeReaderErrorOccurred -- an event that should be sent if an error occured while scanning data
    //  PosEventType::StatusUpdated -- an event to indicate changes to power state
    //
    // Additionally, the following event is sent to the runtime, but is handled entirely by PosCx
    //  PosEventType::ReleaseDeviceRequested
    
    // The following shows an example of sending MSR data

    MSR_DATA_RECEIVED dataReceivedEventInfo;
    
    // Fill in all the fields in MSR_DATA_RECEIVED

    // This call actually pends the data to be send to the WinRT APIs.
    NTSTATUS status = PosCxPutPendingEvent(Device, MSR_INTERFACE_TAG, PosEventType::MagneticStripeReaderDataReceived, sizeof(dataReceivedEventInfo), &dataReceivedEventInfo, POS_CX_EVENT_ATTR_DATA);

    if (!NT_SUCCESS(status))
    {
        // This should only happen in rare cases such as out of memory (or that the device or interface tag isn't found).  The driver should most likely drop the event.
    }
}