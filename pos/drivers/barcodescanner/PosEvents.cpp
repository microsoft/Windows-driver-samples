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
VOID EvtOnBarcodeScanDataRetrieved(_In_ WDFDEVICE Device)
{
    // Events that the runtime can handle for a barcode scanner device are:
    //  PosEventType::BarcodeScannerDataReceived -- the standard event with barcode scan data
    //  PosEventType::BarcodeScannerErrorOccurred -- an event that should be sent if an error occured while scanning data
    //  PosEventType::BarcodeScannerImagePreviewReceived -- an event that sends a complete .bmp image to the runtime for imagers
    //  PosEventType::BarcodeScannerTriggerPressed -- an event indicating that the trigger on the device has been pushed and the device is looking for data
    //  PosEventType::BarcodeScannerTriggerReleased -- an event indicating that the trigger on the device has been released and the device is no longer looking for data
    //  PosEventType::StatusUpdated -- an event to indicate changes to power state
    //
    // Additionally, the following event is sent to the runtime, but is handled entirely by PosCx
    //  PosEventType::ReleaseDeviceRequested
    
    // The following shows an example of sending barcode scan data
    CHAR exampleData[] = "]0A12345";
    CHAR exampleDataLabel[] = "12345";

    // total size is the struct plus the size of the two strings (minus the null terminators which aren't transmitted).
    size_t totalSize = sizeof(PosBarcodeScannerDataReceivedEventData) + sizeof(exampleData) - sizeof(CHAR) + sizeof(exampleDataLabel) - sizeof(CHAR);

    
    // PosCx supports two methods of pending the event data -- one where it takes the WDFMEMORY for the event, and the other where it creates it
    // The subtle difference between the two is that the one that takes the WDFMEMORY must have the event header information already added.
    // 
    // Since that's the case with the PosBarcodeScannerDataReceivedEventData data structure, barcode scanner drivers should create the WDFMEMORY objects
    // and pass them to PosCx.

    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Device;

    BYTE* eventData = nullptr;
    WDFMEMORY eventMemory = NULL;
    NTSTATUS status = WdfMemoryCreate(
        &Attributes,
        NonPagedPoolNx,
        (ULONG)'eSOP',
        totalSize,
        &eventMemory,
        (PVOID*)&eventData
        );

    if (!NT_SUCCESS(status))
    {
        // handle out of memory error
        return;
    }

    // Calculate data and label sizes
    size_t exampleDataByteCount = sizeof(exampleData) - sizeof(CHAR);
    size_t exampleLabelByteCount = sizeof(exampleDataLabel) - sizeof(CHAR);
    
    PosBarcodeScannerDataReceivedEventData* eventHeader = (PosBarcodeScannerDataReceivedEventData*)eventData;
    eventHeader->Header.EventType = PosEventType::BarcodeScannerDataReceived;
    eventHeader->Header.DataLength = (UINT32)totalSize;
    eventHeader->DataType = BarcodeSymbology::Ean13;
    eventHeader->ScanDataLength = (UINT32)exampleDataByteCount;
    eventHeader->ScanDataLabelLength = (UINT32)exampleLabelByteCount;

    // These use memcpy because wcscpy would append the null terminator and the event data doesn't use it
    BYTE* eventScanData = eventData + sizeof(PosBarcodeScannerDataReceivedEventData);
    memcpy(eventScanData, exampleData, exampleDataByteCount);
    BYTE* eventLabelData = eventScanData + exampleDataByteCount;
    memcpy(eventLabelData, exampleDataLabel, exampleLabelByteCount);

    // This call actually pends the data to be send to the WinRT APIs.
    status = PosCxPutPendingEventMemory(Device, SCANNER_INTERFACE_TAG, eventMemory, POS_CX_EVENT_ATTR_DATA);

    if (!NT_SUCCESS(status))
    {
        // This should only happen in rare cases such as out of memory (or that the device or interface tag isn't found).  The driver should most likely drop the event.
        WdfObjectDelete(eventMemory);
    }
}