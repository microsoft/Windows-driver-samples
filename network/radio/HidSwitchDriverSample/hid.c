#include <RadioSwitchHidUsbFx2.h>
#include "hid.tmh"

// Radio Management Collection - Scenario 1 control collection (Button only)
//
const HID_REPORT_DESCRIPTOR         c_ButtonDescriptor[] = {
    0x05, GENERIC_DESKTOP_USAGE_PAGE,   // USAGE_PAGE (Generic Desktop)
    0x09, WIRELESS_RADIO_CONTROLS_USAGE,// USAGE (Wireless Radio Controls)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, GENERIC_DESKTOP_REPORT_ID,    //   REPORT_ID
    0x15, 0x00,                         //   LOGICAL_MINIMUM(0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM(1)
    //
    0x09, WIRELESS_RADIO_BUTTON_USAGE,  //   USAGE (Wireless Radio Button)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x81, 0x06,                         //   INPUT (Data, Variable, Rel) - button bit
    0x75, 0x07,                         //   REPORT_SIZE (7)
    0x81, 0x03,                         //   INPUT (const, var, abs) -7 bits of padding
    0xC0                                // END_COLLECTION
};


// Radio Management Collection - Scenario 2 control collection (Button with LED)
//
const HID_REPORT_DESCRIPTOR         c_ButtonWithLedDescriptor[] = {
    0x05, GENERIC_DESKTOP_USAGE_PAGE,   // USAGE_PAGE (Generic Desktop)
    0x09, WIRELESS_RADIO_CONTROLS_USAGE,// USAGE (Wireless Radio Controls)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, GENERIC_DESKTOP_REPORT_ID,    //   REPORT_ID
    0x15, 0x00,                         //   LOGICAL_MINIMUM(0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM(1)
    //
    0x09, WIRELESS_RADIO_BUTTON_USAGE,  //   USAGE (Wireless Radio Button)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x81, 0x06,                         //   INPUT (Data, Variable, Rel) - button bit
    0x75, 0x07,                         //   REPORT_SIZE (7)
    0x81, 0x03,                         //   INPUT (const, var, abs) -7 bits of padding
    //
    0x09, WIRELESS_RADIO_LED_USAGE,     //   USAGE (Wireless Radio LED)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x91, 0x02,                         //   OUTPUT (Data, Variable, Rel) -led bit
    0x75, 0x07,                         //   REPORT_SIZE (7)
    0x91, 0x03,                         //   OUTPUT (const, var, abs) -7 bits of padding
    0xC0                                // END_COLLECTION
};


// Radio Management Collection - Scenario 3 control collection (Slider Switch)
//
const HID_REPORT_DESCRIPTOR         c_SliderSwitchDescriptor[] = {
    0x05, GENERIC_DESKTOP_USAGE_PAGE,   // USAGE_PAGE (Generic Desktop)
    0x09, WIRELESS_RADIO_CONTROLS_USAGE,// USAGE (Wireless Radio Controls)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, GENERIC_DESKTOP_REPORT_ID,    //   REPORT_ID
    0x15, 0x00,                         //   LOGICAL_MINIMUM(0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM(1)
    //
    0x09, WIRELESS_RADIO_SLIDER_USAGE,  //   USAGE (Wireless Radio Slider Switch)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x81, 0x02,                         //   INPUT (Data, Variable, Abs) -switch bit
    0x75, 0x07,                         //   REPORT_SIZE (7)
    0x81, 0x03,                         //   INPUT (const, var, abs) -7 bits of padding
    0xC0                                // END_COLLECTION
};


// Radio Management Collection - Scenario 4 control collection (Slider Switch with LED)
//
const HID_REPORT_DESCRIPTOR         c_SliderSwitchWithLedDescriptor[] = {
   0x05, GENERIC_DESKTOP_USAGE_PAGE,   // USAGE_PAGE (Generic Desktop)
   0x09, WIRELESS_RADIO_CONTROLS_USAGE,// USAGE (Wireless Radio Controls)
   0xA1, 0x01,                         // COLLECTION (Application)
   0x85, GENERIC_DESKTOP_REPORT_ID,    //   REPORT_ID
   0x15, 0x00,                         //   LOGICAL_MINIMUM(0)
   0x25, 0x01,                         //   LOGICAL_MAXIMUM(1)
   //
   0x09, WIRELESS_RADIO_SLIDER_USAGE,  //   USAGE (Wireless Radio Slider Switch)
   0x95, 0x01,                         //   REPORT_COUNT (1)
   0x75, 0x01,                         //   REPORT_SIZE (1)
   0x81, 0x02,                         //   INPUT (Data, Variable, Abs) -switch bit
   0x75, 0x07,                         //   REPORT_SIZE (7)
   0x81, 0x03,                         //   INPUT (const, var, abs) -7 bits of padding
    //
   0x09, WIRELESS_RADIO_LED_USAGE,     //   USAGE (Wireless Radio LED)
   0x75, 0x01,                         //   REPORT_SIZE (1)
   0x91, 0x02,                         //   OUTPUT (Data, Variable, Rel) -led bit
   0x75, 0x07,                         //   REPORT_SIZE (7)
   0x91, 0x03,                         //   OUTPUT (const, var, abs) -7 bits of padding
   0xC0                                // END_COLLECTION
};


// Radio Management Collection - Scenario 5 control collection (LED only)
//
const HID_REPORT_DESCRIPTOR         c_LedOnlyDescriptor[] = {
    0x05, GENERIC_DESKTOP_USAGE_PAGE,   // USAGE_PAGE (Generic Desktop)
    0x09, WIRELESS_RADIO_CONTROLS_USAGE,// USAGE (Wireless Radio Controls)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, GENERIC_DESKTOP_REPORT_ID,    //   REPORT_ID
    0x15, 0x00,                         //   LOGICAL_MINIMUM(0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM(1)
    //
    0x09, WIRELESS_RADIO_LED_USAGE,     //   USAGE (Wireless Radio LED)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x91, 0x02,                         //   OUTPUT (Data, Variable, Rel) -led bit
    0x75, 0x07,                         //   REPORT_SIZE (7)
    0x91, 0x03,                         //   OUTPUT (const, var, abs) -7 bits of padding
    0xC0                                // END_COLLECTION
};


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, HidFx2SetOutput)
    #pragma alloc_text(PAGE, HidFx2GetInput)
    #pragma alloc_text(PAGE, SendVendorCommand)
#endif


// This event is called when the framework receives IRP_MJ_INTERNAL DEVICE_CONTROL requests from the system.
//
void HidFx2EvtInternalDeviceControl(
    _In_ WDFQUEUE     hQueue,
    _In_ WDFREQUEST   hRequest,
    _In_ size_t       cOutputBufferLength,
    _In_ size_t       cInputBufferLength,
    _In_ ULONG        ulIoControlCode
   )
{
    NTSTATUS            status = STATUS_SUCCESS;
    WDFDEVICE           hDevice;
    PDEVICE_EXTENSION   pDevContext = NULL;

    UNREFERENCED_PARAMETER(cOutputBufferLength);
    UNREFERENCED_PARAMETER(cInputBufferLength);

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Enter\n");

    hDevice = WdfIoQueueGetDevice(hQueue);
    pDevContext = GetDeviceContext(hDevice);

    TraceInfo(DBG_IOCTL, "(%!FUNC!) Queue:0x%p, Request:0x%p\n", hQueue, hRequest);

    switch (ulIoControlCode)
    {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        TraceInfo(DBG_IOCTL, "IOCTL_HID_GET_DEVICE_DESCRIPTOR\n");
        status = HidFx2GetHidDescriptor(hDevice, hRequest);
        WdfRequestComplete(hRequest, status);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        TraceInfo(DBG_IOCTL, "IOCTL_HID_GET_DEVICE_ATTRIBUTES\n");
        status = HidFx2GetDeviceAttributes(hRequest);
        WdfRequestComplete(hRequest, status);
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        TraceInfo(DBG_IOCTL, "IOCTL_HID_GET_REPORT_DESCRIPTOR\n");
        status = HidFx2GetReportDescriptor(hDevice, hRequest);
        WdfRequestComplete(hRequest, status);
        break;

    case IOCTL_HID_READ_REPORT:
        TraceInfo(DBG_IOCTL, "IOCTL_HID_READ_REPORT\n");
        status = WdfRequestForwardToIoQueue(hRequest, pDevContext->hInterruptMsgQueue);
        if (!NT_SUCCESS(status))
        {
            TraceErr(DBG_IOCTL, "WdfRequestForwardToIoQueue failed with status: %!STATUS!\n", status);
            WdfRequestComplete(hRequest, status);
        }
        break;

    case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        TraceInfo(DBG_IOCTL, "IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST\n");
        status = HidFx2SendIdleNotification(hRequest);
        if (!NT_SUCCESS(status))
        {
            TraceErr(DBG_IOCTL, "SendIdleNotification failed with status: %!STATUS!\n", status);
            WdfRequestComplete(hRequest, status);
        }
        break;

    case IOCTL_HID_GET_INPUT_REPORT:
        TraceInfo(DBG_IOCTL, "IOCTL_HID_GET_INPUT_REPORT\n");
        HidFx2GetInput(hRequest);
        WdfRequestComplete(hRequest, status);
        break;

    case IOCTL_HID_SET_OUTPUT_REPORT:
        TraceInfo(DBG_IOCTL, "IOCTL_HID_SET_OUTPUT_REPORT\n");
        status = HidFx2SetOutput(hRequest);
        WdfRequestComplete(hRequest, status);
        break;

    default:
        TraceInfo(DBG_IOCTL, "IOCTL Not Supported 0x%X\n", ulIoControlCode);
        status = STATUS_NOT_SUPPORTED;
        WdfRequestComplete(hRequest, status);
        break;
    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit\n");
    return;
}



NTSTATUS HidFx2GetInput(_In_ WDFREQUEST hRequest)
{
    NTSTATUS                    status = STATUS_SUCCESS;
    WDF_REQUEST_PARAMETERS      params;
    WDFDEVICE                   hDevice;
    PHID_XFER_PACKET            pTransferPacket = NULL;
    PHIDFX2_IO_REPORT           pOutputReport = NULL;
    unsigned char               bSwitchState = 0;

    PAGED_CODE();

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Enter\n");

    hDevice = WdfIoQueueGetDevice(WdfRequestGetIoQueue(hRequest));
    
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(hRequest, &params);


    if (params.Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_XFER_PACKET))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TraceErr(DBG_IOCTL, "(%!FUNC!) Userbuffer is small %!STATUS!\n", status);
        return status;
    }

    pTransferPacket = (PHID_XFER_PACKET) WdfRequestWdmGetIrp(hRequest)->UserBuffer;
    if (pTransferPacket == NULL)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        TraceErr(DBG_IOCTL, "(%!FUNC!) Irp->UserBuffer is NULL %!STATUS!\n", status);
        return status;
    }

    if (pTransferPacket->reportBufferLen == 0)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TraceErr(DBG_IOCTL, "(%!FUNC!) HID_XFER_PACKET->reportBufferLen is 0, %!STATUS!\n", status);
        return status;
    }

    if (pTransferPacket->reportBufferLen < sizeof(UCHAR))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TraceErr(DBG_IOCTL, "(%!FUNC!) HID_XFER_PACKET->reportBufferLen is too small, %!STATUS!\n", status);
        return status;
    }

    if (pTransferPacket->reportId != GENERIC_DESKTOP_REPORT_ID)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        TraceErr(DBG_IOCTL, "(%!FUNC!) Incorrect report ID, %!STATUS!\n", status);
        return status;
    }

    pOutputReport = (PHIDFX2_IO_REPORT)pTransferPacket->reportBuffer;

    // Get the switch state directly from the hardware
    status = HidFx2GetSwitchState(hDevice, &bSwitchState);
    TraceVerbose(DBG_IOCTL, "(%!FUNC!) switch state 0x%x\n", bSwitchState);

    //Mask off everything except the actual switch bit
    bSwitchState &= RADIO_SWITCH_BUTTONS_BIT_MASK;
    TraceVerbose(DBG_IOCTL, "(%!FUNC!) maskedswitch state 0x%x\n", bSwitchState);

    pOutputReport->bReportId = GENERIC_DESKTOP_REPORT_ID;
    pOutputReport->bData = bSwitchState;

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit status %!STATUS!\n", status);
    return status;
}



NTSTATUS HidFx2SetOutput(_In_ WDFREQUEST hRequest)
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PHID_XFER_PACKET            pTransferPacket = NULL;
    WDF_REQUEST_PARAMETERS      params;
    PHIDFX2_IO_REPORT           pOutputReport = NULL;
    WDFDEVICE                   hDevice;

    PAGED_CODE();

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Enter\n");

    hDevice = WdfIoQueueGetDevice(WdfRequestGetIoQueue(hRequest));
    
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(hRequest, &params);

    if (params.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TraceErr(DBG_IOCTL, "(%!FUNC!) Userbuffer is small %!STATUS!\n", status);
        return status;
    }

    pTransferPacket = (PHID_XFER_PACKET) WdfRequestWdmGetIrp(hRequest)->UserBuffer;
    if (pTransferPacket == NULL)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        TraceErr(DBG_IOCTL, "(%!FUNC!) Irp->UserBuffer is NULL %!STATUS!\n", status);
        return status;
    }

    if (pTransferPacket->reportBufferLen == 0)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TraceErr(DBG_IOCTL, "(%!FUNC!) HID_XFER_PACKET->reportBufferLen is 0, %!STATUS!\n", status);
        return status;
    }

    if (pTransferPacket->reportBufferLen < sizeof(UCHAR))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TraceErr(DBG_IOCTL, "(%!FUNC!) HID_XFER_PACKET->reportBufferLen is too small, %!STATUS!\n", status);
        return status;
    }

    if (pTransferPacket->reportId != GENERIC_DESKTOP_REPORT_ID)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        TraceErr(DBG_IOCTL, "(%!FUNC!) Incorrect report ID, %!STATUS!\n", status);
        return status;
    }
    
    pOutputReport = (PHIDFX2_IO_REPORT)pTransferPacket->reportBuffer;

    if (pOutputReport->bData == 0)
    {
        status = SendVendorCommand(hDevice, HIDFX2_SET_BARGRAPH_DISPLAY, BARGRAPH_LED_ALL_OFF);
    }
    else
    {
        status = SendVendorCommand(hDevice, HIDFX2_SET_BARGRAPH_DISPLAY, BARGRAPH_LED_ALL_ON);
    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit status %!STATUS!\n", status);
    return status;
}



//This routine sets the state of the Feature: in this case Segment Display on the USB FX2 board.
NTSTATUS SendVendorCommand(_In_ WDFDEVICE hDevice, _In_ unsigned char bVendorCommand, _In_ unsigned char bCommandData)
{
    NTSTATUS                     status = STATUS_SUCCESS;
    ULONG                        cBytesTransferred = 0;
    PDEVICE_EXTENSION            pDevContext = NULL;
    WDF_MEMORY_DESCRIPTOR        memDesc;
    WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS     sendOptions;

    PAGED_CODE();

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Enter\n");

    pDevContext = GetDeviceContext(hDevice);

    TraceInfo(DBG_IOCTL, "(%!FUNC!): Command:0x%x, data: 0x%x\n", bVendorCommand, bCommandData);

    // Send the I/O with a timeout.
    // We do that because we send the I/O in the context of the user thread and if it gets stuck, it would prevent the user process from existing.
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_SEC(5));

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                             BmRequestHostToDevice,
                                             BmRequestToDevice,
                                             bVendorCommand,    // Request
                                             0,                 // Value
                                             0);                // Index

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc, &bCommandData, sizeof(bCommandData));

    status = WdfUsbTargetDeviceSendControlTransferSynchronously(pDevContext->hUsbDevice,
                                                                WDF_NO_HANDLE,      // Optional WDFREQUEST
                                                                &sendOptions,       // PWDF_REQUEST_SEND_OPTIONS
                                                                &controlSetupPacket,
                                                                &memDesc,
                                                                &cBytesTransferred);
    if (!NT_SUCCESS(status))
    {
        TraceErr(DBG_IOCTL, "(%!FUNC!): Failed to set Segment Display state - %!STATUS!\n", status);
    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit\n");

    return status;
}



// Finds the HID descriptor and copies it into the buffer provided by the Request.
//
NTSTATUS HidFx2GetHidDescriptor(_In_ WDFDEVICE hDevice, _In_ WDFREQUEST hRequest)
{
    NTSTATUS            status = STATUS_SUCCESS;
    size_t              cBytesToCopy = 0;
    WDFMEMORY           memory;
    PDEVICE_EXTENSION   pDevContext = NULL;
    HID_DESCRIPTOR      hidDescriptor;


    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Entry\n");

    status = WdfRequestRetrieveOutputMemory(hRequest, &memory);
    if (NT_SUCCESS(status))
    {
        hidDescriptor.bLength= sizeof(hidDescriptor);               // length of HID descriptor
        hidDescriptor.bDescriptorType= HID_HID_DESCRIPTOR_TYPE;     // descriptor type == HID  0x21
        hidDescriptor.bcdHID= HID_CLASS_SPEC_RELEASE_1_00;          // hid spec release
        hidDescriptor.bCountry= HID_COUNTRY_NON_LOCALIZED;          // country code
        hidDescriptor.bNumDescriptors= 0x01;                        // number of HID class descriptors
        hidDescriptor.DescriptorList[0].bReportType =HID_REPORT_DESCRIPTOR_TYPE;
        hidDescriptor.DescriptorList[0].wReportLength = sizeof(c_ButtonDescriptor);

        pDevContext = GetDeviceContext(hDevice);
        if (pDevContext != NULL)
        {
        // Set length of non default report descriptor
            switch (pDevContext->driverMode)
            {
            case DM_BUTTON_AND_LED:
                hidDescriptor.DescriptorList[0].wReportLength = sizeof(c_ButtonWithLedDescriptor);
                break;
            case DM_SLIDER_SWITCH:
                hidDescriptor.DescriptorList[0].wReportLength = sizeof(c_SliderSwitchDescriptor);
                break;
            case DM_SLIDER_SWITCH_AND_LED:
                hidDescriptor.DescriptorList[0].wReportLength = sizeof(c_SliderSwitchWithLedDescriptor);
                break;
            case DM_LED_ONLY:
                hidDescriptor.DescriptorList[0].wReportLength = sizeof(c_LedOnlyDescriptor);
                break;
            default:
                break;
            }
        }
        cBytesToCopy = hidDescriptor.bLength;
        if (cBytesToCopy != 0)
        {
            status = WdfMemoryCopyFromBuffer(memory, 0, (PVOID)&hidDescriptor, cBytesToCopy);
            if (NT_SUCCESS(status))
            {
                // Report how many bytes were copied
                WdfRequestSetInformation(hRequest, cBytesToCopy);
            }
            else // WdfMemoryCopyFromBuffer failed
            {
                TraceErr(DBG_IOCTL, "(%!FUNC!) WdfMemoryCopyFromBuffer failed %!STATUS!\n", status);
                return status;
            }
        }
        else // length is zero
        {
            status = STATUS_INVALID_DEVICE_STATE;
            TraceErr(DBG_IOCTL, "(%!FUNC!) c_DefaultHidDescriptor is zero, %!STATUS!\n", status);
            return status;
        }
        
    }
    else // WdfRequestRetrieveOutputMemory failed
    {
        TraceErr(DBG_IOCTL, " (%!FUNC!) WdfRequestRetrieveOutputMemory failed %!STATUS!\n", status);
        return status;
    }

    TraceVerbose(DBG_IOCTL, " (%!FUNC!) HidFx2GetHidDescriptor Exit = %!STATUS!\n", status);
    return status;
}



// Finds the Report descriptor and copies it into the buffer provided by the Request.
//
NTSTATUS HidFx2GetReportDescriptor(_In_ WDFDEVICE hDevice, _In_ WDFREQUEST hRequest)
{
    NTSTATUS                status = STATUS_SUCCESS;
    WDFMEMORY               memory;
    PDEVICE_EXTENSION       pDevContext = NULL;
    PVOID                   pBuffer = (PVOID)c_ButtonDescriptor;
    size_t                  cBytesToCopy = sizeof(c_ButtonDescriptor);


    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Entry\n");

    status = WdfRequestRetrieveOutputMemory(hRequest, &memory);
    if (NT_SUCCESS(status))
    {
        pDevContext = GetDeviceContext(hDevice);
        if (pDevContext != NULL)
        {
           // Select a non default report descriptor
            switch (pDevContext->driverMode)
            {
            case DM_BUTTON_AND_LED:
                pBuffer = (PVOID)c_ButtonWithLedDescriptor;
                cBytesToCopy = sizeof(c_ButtonWithLedDescriptor);
                break;
            case DM_SLIDER_SWITCH:
                pBuffer = (PVOID)c_SliderSwitchDescriptor;
                cBytesToCopy = sizeof(c_SliderSwitchDescriptor);
                break;
            case DM_SLIDER_SWITCH_AND_LED:
                pBuffer = (PVOID)c_SliderSwitchWithLedDescriptor;
                cBytesToCopy = sizeof(c_SliderSwitchWithLedDescriptor);
                break;
            case DM_LED_ONLY:
                pBuffer = (PVOID)c_LedOnlyDescriptor;
                cBytesToCopy = sizeof(c_LedOnlyDescriptor);
                break;
            default:
                break;
            }
        }
        if (cBytesToCopy != 0)
        {
            status = WdfMemoryCopyFromBuffer(memory, 0, pBuffer, cBytesToCopy);
            if (NT_SUCCESS(status))
            {
                // Report how many bytes were copied
                WdfRequestSetInformation(hRequest, cBytesToCopy);
            }
            else // WdfMemoryCopyFromBuffer failed
            {
                TraceErr(DBG_IOCTL, "(%!FUNC!) WdfMemoryCopyFromBuffer failed %!STATUS!\n", status);
            }
        }
        else // report length is zero
        {
            status = STATUS_INVALID_DEVICE_STATE;
            TraceErr(DBG_IOCTL, "(%!FUNC!) c_DefaultHidDescriptor's reportLength is zero, %!STATUS!\n", status);
        }
    }
    else // WdfRequestRetrieveOutputMemory failed
    {
        TraceErr(DBG_IOCTL, "(%!FUNC!)WdfRequestRetrieveOutputMemory failed %!STATUS!\n", status);

    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit = %!STATUS!\n", status);
    return status;
}



// Fill in the given struct _HID_DEVICE_ATTRIBUTES
//
NTSTATUS HidFx2GetDeviceAttributes(_In_ WDFREQUEST hRequest)
{
    NTSTATUS                 status = STATUS_SUCCESS;
    PHID_DEVICE_ATTRIBUTES   pDeviceAttributes = NULL;
    PUSB_DEVICE_DESCRIPTOR   pUsbDeviceDescriptor = NULL;
    PDEVICE_EXTENSION        pDeviceInfo = NULL;

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Entry\n");

    pDeviceInfo = GetDeviceContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(hRequest)));
    status = WdfRequestRetrieveOutputBuffer(hRequest, sizeof (HID_DEVICE_ATTRIBUTES), &pDeviceAttributes, NULL);
    if (NT_SUCCESS(status))
    {
        // Retrieve USB device descriptor saved in device context
        pUsbDeviceDescriptor = WdfMemoryGetBuffer(pDeviceInfo->hDeviceDescriptor, NULL);
        pDeviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
        pDeviceAttributes->VendorID = pUsbDeviceDescriptor->idVendor;
        pDeviceAttributes->ProductID = pUsbDeviceDescriptor->idProduct;
        pDeviceAttributes->VersionNumber = pUsbDeviceDescriptor->bcdDevice;
        // Report how many bytes were copied
        WdfRequestSetInformation(hRequest, sizeof (HID_DEVICE_ATTRIBUTES));
    }
    else // WdfRequestRetrieveOutputBuffer failed
    {
        TraceErr(DBG_IOCTL, "(%!FUNC!) WdfRequestRetrieveOutputBuffer failed %!STATUS!\n", status);
    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit = %!STATUS!\n", status);
    return status;
}



// Pass down Idle notification request to lower driver
//
NTSTATUS HidFx2SendIdleNotification(_In_ WDFREQUEST hRequest)
{
    NTSTATUS                   status = STATUS_SUCCESS;
    WDF_REQUEST_SEND_OPTIONS   options;
    WDFIOTARGET                hNextLowerDriver;
    WDFDEVICE                  hDevice;
    PIO_STACK_LOCATION         pCurrentIrpStack = NULL;
    IO_STACK_LOCATION          nextIrpStack;

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Entry\n");

    hDevice = WdfIoQueueGetDevice(WdfRequestGetIoQueue(hRequest));
    pCurrentIrpStack = IoGetCurrentIrpStackLocation(WdfRequestWdmGetIrp(hRequest));

    // Convert the request to corresponding USB Idle notification request
    if (pCurrentIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO))
    {
        ASSERT(sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) == sizeof(USB_IDLE_CALLBACK_INFO));
        #pragma warning(suppress :4127)  // conditional expression is constant warning
        if (sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) == sizeof(USB_IDLE_CALLBACK_INFO))
        {
            // prepare next stack location
            RtlZeroMemory(&nextIrpStack, sizeof(IO_STACK_LOCATION));
            nextIrpStack.MajorFunction = pCurrentIrpStack->MajorFunction;
            nextIrpStack.Parameters.DeviceIoControl.InputBufferLength = pCurrentIrpStack->Parameters.DeviceIoControl.InputBufferLength;
            nextIrpStack.Parameters.DeviceIoControl.Type3InputBuffer = pCurrentIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
            nextIrpStack.Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
            nextIrpStack.DeviceObject = WdfIoTargetWdmGetTargetDeviceObject(WdfDeviceGetIoTarget(hDevice));

            // Format the I/O request for the driver's local I/O target by using the contents of the specified WDM I/O stack location structure.
            WdfRequestWdmFormatUsingStackLocation(hRequest, &nextIrpStack);

            // Send the request down using Fire and forget option.
            WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
            hNextLowerDriver = WdfDeviceGetIoTarget(hDevice);
            if (WdfRequestSend(hRequest, hNextLowerDriver, &options) == FALSE)
            {
                status = STATUS_UNSUCCESSFUL;
            }
        }
        else // Incorrect DeviceIoControl.InputBufferLength
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            TraceErr(DBG_IOCTL, "(%!FUNC!) Incorrect DeviceIoControl.InputBufferLength, %!STATUS!\n", status);
            return status;
        }
    }
    else // DeviceIoControl.InputBufferLength too small
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TraceErr(DBG_IOCTL, "(%!FUNC!) DeviceIoControl.InputBufferLength too small, %!STATUS!\n", status);
        return status;
    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit = %!STATUS!\n", status);
    return status;
}





