#include <RadioSwitchHidUsbFx2.h>
#include "usb.tmh"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HidFx2EvtDevicePrepareHardware)
#pragma alloc_text(PAGE, HidFx2EvtDeviceD0Exit)
#pragma alloc_text(PAGE, HidFx2ConfigContReaderForInterruptEndPoint)
#endif


// In this callback, the driver does whatever is necessary to make the hardware ready to use.
// In the case of a USB device, this involves reading and selecting descriptors.
NTSTATUS
HidFx2EvtDevicePrepareHardware(
    _In_ WDFDEVICE    hDevice,
    _In_ WDFCMRESLIST hResourceList,
    _In_ WDFCMRESLIST hResourceListTranslated
   )
{
    NTSTATUS                            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION                   pDevContext = NULL;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    WDF_OBJECT_ATTRIBUTES               attributes;
    PUSB_DEVICE_DESCRIPTOR              pUsbDeviceDescriptor = NULL;

    UNREFERENCED_PARAMETER(hResourceList);
    UNREFERENCED_PARAMETER(hResourceListTranslated);

    PAGED_CODE ();

    TraceVerbose(DBG_INIT, "(%!FUNC!) Enter\n");

    pDevContext = GetDeviceContext(hDevice);

    // Create a WDFUSBDEVICE object. WdfUsbTargetDeviceCreate obtains the USB device descriptor and the first USB configuration
    //descriptor from the device and stores them. It also creates a framework USB interface object for each interface in the device's first configuration.
    //
    // The parent of each USB device object is the driver's framework driver object. The driver cannot change this parent, and the ParentObject
    // member or the WDF_OBJECT_ATTRIBUTES structure must be NULL.
    //
    // We only create device the first time PrepareHardware is called. If the device is restarted by pnp manager for resource rebalance, we 
    // will use the same device handle but then select the interfaces again because the USB stack could reconfigure the device on restart.
    if (pDevContext->hUsbDevice == NULL)
    {
        status = WdfUsbTargetDeviceCreate(hDevice, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->hUsbDevice);
        if (!NT_SUCCESS(status))
        {
            TraceErr(DBG_PNP, "(%!FUNC!) WdfUsbTargetDeviceCreate failed %!STATUS!\n", status);
            return status;
        }
    }

    // Select a device configuration by using a WDF_USB_DEVICE_SELECT_CONFIG_PARAMS
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
    status = WdfUsbTargetDeviceSelectConfig(pDevContext->hUsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);
    if (!NT_SUCCESS(status))
    {
        TraceErr(DBG_PNP, "(%!FUNC!) WdfUsbTargetDeviceSelectConfig failed %!STATUS!\n", status);
        return status;
    }

    pDevContext->hUsbInterface = configParams.Types.SingleInterface.ConfiguredUsbInterface;

    // Get the device descriptor and store it in device context
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = hDevice;
    status = WdfMemoryCreate(&attributes,
                             NonPagedPool,
                             0,
                             sizeof(USB_DEVICE_DESCRIPTOR),
                             &pDevContext->hDeviceDescriptor,
                             &pUsbDeviceDescriptor);
    if (!NT_SUCCESS(status))
    {
        TraceErr(DBG_PNP, "(%!FUNC!) WdfMemoryCreate for Device Descriptor failed %!STATUS!\n", status);
        return status;
    }

    WdfUsbTargetDeviceGetDeviceDescriptor(pDevContext->hUsbDevice, pUsbDeviceDescriptor);

    // Get the Interrupt pipe. There are other endpoints but we are only interested in interrupt endpoint since our HID data comes from this
    pDevContext->hInterruptPipe = WdfUsbInterfaceGetConfiguredPipe(pDevContext->hUsbInterface, INTERRUPT_ENDPOINT_INDEX, NULL);
    if (NULL == pDevContext->hInterruptPipe)
    {
        TraceErr(DBG_PNP, "(%!FUNC!) Failed to get interrupt pipe info\n");
        status = STATUS_INVALID_DEVICE_STATE;
        return status;
    }

    // Tell the framework that it's okay to read less than MaximumPacketSize
    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDevContext->hInterruptPipe);

    //configure continuous reader
    status = HidFx2ConfigContReaderForInterruptEndPoint(pDevContext);

    TraceVerbose(DBG_INIT, "(%!FUNC!) Exit, Status: %!STATUS!\n", status);

    return status;
}



// This routine configures a continuous reader on the interrupt endpoint. It's called from the PrepareHarware event.
//
NTSTATUS HidFx2ConfigContReaderForInterruptEndPoint(PDEVICE_EXTENSION pDeviceContext)
{
    WDF_USB_CONTINUOUS_READER_CONFIG    contReaderConfig;
    NTSTATUS                            status = STATUS_SUCCESS;

    PAGED_CODE ();

    TraceVerbose(DBG_INIT, "(%!FUNC!) Enter\n");

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&contReaderConfig,
                                          HidFx2EvtUsbInterruptPipeReadComplete,
                                          pDeviceContext,    // Context
                                          sizeof(UCHAR));   // TransferLength

    status = WdfUsbTargetPipeConfigContinuousReader(pDeviceContext->hInterruptPipe, &contReaderConfig);
    if (!NT_SUCCESS(status))
    {
        TraceErr(DBG_INIT, "(%!FUNC!) failed %!STATUS!\n", status);
    }

    TraceVerbose(DBG_INIT, "(%!FUNC!) Exit, status: %!STATUS!\n", status);

    return status;
}



// This the completion routine of the continuous reader. This can called concurrently on multiprocessor system if there are
// more than one readers configured. So make sure to protect access to global resources.
//
void HidFx2EvtUsbInterruptPipeReadComplete(
    WDFUSBPIPE  hPipe,
    WDFMEMORY   hBuffer,
    size_t      cNumBytesTransferred,
    WDFCONTEXT  pContext
   )
{
    PDEVICE_EXTENSION   pDevContext = pContext;
    BOOLEAN             fInTimerQueue;
    unsigned char       *pbSwitchState = NULL;
    unsigned char       bCurrentSwitchState = 0;
    unsigned char       bPrevSwitchState = 0;
    unsigned char       bToggledSwitch = 0;

    UNREFERENCED_PARAMETER(cNumBytesTransferred);
    UNREFERENCED_PARAMETER(hPipe);

    TraceVerbose(DBG_INIT, "(%!FUNC!) Enter\n");

    // Interrupt endpoints sends switch state when first started  or when resuming from suspend.
    // We need to ignore that data since user did not change the switch state.
    if (pDevContext->fIsPowerUpSwitchState)
    {
        pDevContext->fIsPowerUpSwitchState = FALSE;
        TraceInfo(DBG_INIT, "(%!FUNC!) Dropping interrupt message since received during powerup/resume\n");
        return;
    }


    // Make sure that there is data in the read packet. 
    // Depending on the device specification, it is possible for it to return a 0 length read in certain conditions.
    if (cNumBytesTransferred == 0)
    {
        TraceWarning(DBG_INIT, "(%!FUNC!) Zero length read occured on the Interrupt Pipe's Continuous Reader\n");
        return;
    }

    pbSwitchState = WdfMemoryGetBuffer(hBuffer, NULL);
    bCurrentSwitchState = ~(*pbSwitchState);                // switchs are inverted on hardware boards
    bCurrentSwitchState &= RADIO_SWITCH_BUTTONS_BIT_MASK;   //Mask off everything except the actual radio switch bit
    bPrevSwitchState = pDevContext->bCurrentSwitchState;

    if (bPrevSwitchState ^ bCurrentSwitchState) // make sure we toggled the radio switch
    {
        switch(pDevContext->driverMode)
        {
        // If it's a slider switch we want  0->1 and 1->0 transitions.
        case DM_SLIDER_SWITCH:
        case DM_SLIDER_SWITCH_AND_LED:
            bToggledSwitch = bCurrentSwitchState;
            // A timer is started for 10 ms everytime there is a switch toggled
            fInTimerQueue = WdfTimerStart(pDevContext->hDebounceTimer, WDF_REL_TIMEOUT_IN_MS(SWITCHPACK_DEBOUNCE_TIME));
            TraceInfo(DBG_INIT, "(%!FUNC!) Debounce Timer started. Existing -%!bool!\n", fInTimerQueue);
            break;
        //If it's a button so we only report 0->1 transitions
        case DM_BUTTON:
        case DM_BUTTON_AND_LED:
            bToggledSwitch = (bPrevSwitchState ^ bCurrentSwitchState) & bCurrentSwitchState;
            if (bToggledSwitch != 0)
            {
                // A timer is started for 10 ms everytime there is a switch toggled on
                fInTimerQueue = WdfTimerStart(pDevContext->hDebounceTimer, WDF_REL_TIMEOUT_IN_MS(SWITCHPACK_DEBOUNCE_TIME));
                TraceInfo(DBG_INIT, "(%!FUNC!) Debounce Timer started. Existing -%!bool!\n", fInTimerQueue);
            }
            break;
        // Ignore button presses if LED only
        case DM_LED_ONLY:
        default:
            break;
        }

        // Store switch state in device context
        pDevContext->bCurrentSwitchState = bCurrentSwitchState;
        pDevContext->bLatestToggledSwitch = bToggledSwitch;
    }
    else
    {
        TraceInfo(DBG_INIT, "(%!FUNC!) Not a radio switch toggle\n");
    }
    TraceInfo(DBG_INIT, "(%!FUNC!) Switch 0x%x, prevSwitch:0x%x, X0R:0x%x\n", bCurrentSwitchState, bPrevSwitchState, bToggledSwitch);

    TraceVerbose(DBG_INIT, "(%!FUNC!) Exit\n");
}



// This function gets called when the timeout period of debounce timer elapses.
// It reports a switch change by completing a pending request
//
void HidFx2EvtTimerFunction(_In_ WDFTIMER hTimer)
{
    WDFDEVICE           hDevice = NULL;
    WDFREQUEST          hRequest;
    PDEVICE_EXTENSION   pDevContext = NULL;
    NTSTATUS            status = STATUS_SUCCESS;
    size_t              cBytesReturned = 0;
    unsigned char       bToggledSwitch = 0;
    ULONG               cBytesToCopy = 0;
    PHIDFX2_IO_REPORT   pInputReport = NULL;

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Enter\n");


    hDevice = WdfTimerGetParentObject(hTimer);
    pDevContext = GetDeviceContext(hDevice);

    bToggledSwitch = pDevContext->bLatestToggledSwitch;
    TraceInfo(DBG_IOCTL, "(%!FUNC!) mode %d switch %d\n", pDevContext->driverMode, bToggledSwitch);

    if (!((pDevContext->driverMode == DM_BUTTON || pDevContext->driverMode == DM_BUTTON_AND_LED) && bToggledSwitch == 0))
    {
        // Check if there are any pending requests in the Interrupt Message Queue.
        // If a request is found then complete the pending request.
        status = WdfIoQueueRetrieveNextRequest(pDevContext->hInterruptMsgQueue, &hRequest);
        if (NT_SUCCESS(status))
        {
            cBytesToCopy = sizeof(pInputReport[0]);
            status = WdfRequestRetrieveOutputBuffer(hRequest,
                                                    cBytesToCopy,
                                                    &pInputReport,
                                                    &cBytesReturned); // BufferLength
            if (NT_SUCCESS(status))
            {
                TraceInfo(DBG_IOCTL, "(%!FUNC!) WdfRequestRetrieveOutputBuffer switch %d\n", bToggledSwitch);

                pInputReport->bReportId = GENERIC_DESKTOP_REPORT_ID;
                pInputReport->bData = bToggledSwitch;
                cBytesReturned = cBytesToCopy;
            }
            else // WdfRequestRetrieveOutputBuffer failed
            {
                TraceErr(DBG_IOCTL, "(%!FUNC!) WdfRequestRetrieveOutputBuffer failed with status: %!STATUS!\n", status);
            }

            WdfRequestCompleteWithInformation(hRequest, status, cBytesReturned);

        } 
        else if (status != STATUS_NO_MORE_ENTRIES)
        {
            TraceErr(DBG_IOCTL, "(%!FUNC!) WdfIoQueueRetrieveNextRequest status %!STATUS!\n", status);
        }
    }
    else
    {
        TraceInfo(DBG_IOCTL, "(%!FUNC!) ignore switch");
    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit\n");
}



// This function gets the switch state of the USB device directly from the hardware
//
NTSTATUS HidFx2GetSwitchState(_In_ WDFDEVICE hDevice, _Out_ unsigned char *pbSwitchState)
{
    PDEVICE_EXTENSION            pDevContext = NULL;
    NTSTATUS                     status = STATUS_SUCCESS;
    WDF_MEMORY_DESCRIPTOR        memDesc;
    WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
    ULONG                        cBytesTransferred = 0;

    if(pbSwitchState != NULL)
    {
        *pbSwitchState = 0;
    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Entry\n");

    pDevContext = GetDeviceContext(hDevice);

    // set the segment state on the USB device
    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                             BmRequestDeviceToHost,
                                             BmRequestToDevice,
                                             HIDFX2_READ_SWITCH_STATE, // Request
                                             0, // Value
                                             0); // Index

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc, pbSwitchState, sizeof(*pbSwitchState));

    status = WdfUsbTargetDeviceSendControlTransferSynchronously(pDevContext->hUsbDevice,
                                                                NULL, // Optional WDFREQUEST
                                                                NULL, // PWDF_REQUEST_SEND_OPTIONS
                                                                &controlSetupPacket,
                                                                &memDesc,
                                                                &cBytesTransferred);

    if (!NT_SUCCESS(status))
    {
        TraceErr(DBG_IOCTL, "(%!FUNC!) Failed to read switch state - %!STATUS! \n", status);
    }
    else
    {
        TraceVerbose(DBG_IOCTL, "(%!FUNC!) Switch state is 0x%x\n", *pbSwitchState);
        *pbSwitchState = ~(*pbSwitchState);     // switchs are inverted on hardware boards
        TraceVerbose(DBG_IOCTL, "(%!FUNC!) Switch state is 0x%x\n", *pbSwitchState);
    }

    TraceVerbose(DBG_IOCTL, "(%!FUNC!) Exit, status: %!STATUS!\n", status);
    return status;
}



// EvtDeviceD0Entry event callback must perform any operations that are necessary before the specified device is used.
// Called every time the hardware needs to be initialized/reinitialized.
// This function is not marked pageable because this function is in the device power up path. 
// This function runs at PASSIVE_LEVEL, even though it is not paged.
//
NTSTATUS HidFx2EvtDeviceD0Entry(_In_  WDFDEVICE hDevice, _In_ WDF_POWER_DEVICE_STATE previousState)
{
    PDEVICE_EXTENSION   pDevContext = NULL;
    NTSTATUS            status = STATUS_SUCCESS;
    unsigned char       bSwitchState = 0;
    unsigned char       bMode = 0;

    pDevContext = GetDeviceContext(hDevice);

    TraceVerbose(DBG_PNP, "(%!FUNC!) Enter - coming from %S\n", DbgDevicePowerString(previousState));

    SendVendorCommand(hDevice, HIDFX2_SET_BARGRAPH_DISPLAY, BARGRAPH_LED_ALL_OFF);

    // Retrieve the current switch state and store it in device context
    status = HidFx2GetSwitchState(hDevice, &bSwitchState);
    if (NT_SUCCESS(status))
    {
        // Left most switches define Mode
        bMode = bSwitchState & MODE_SELECTION_BUTTONS_BIT_MASK;
        switch (bMode)
        {
        case SWITCHPACK_SELECTION_FOR_MODE2:
            pDevContext->driverMode = DM_BUTTON_AND_LED;
            SendVendorCommand(hDevice, HIDFX2_SET_7SEGMENT_DISPLAY, SEGMENT_DISPLAY_2);
            break;
        case SWITCHPACK_SELECTION_FOR_MODE3:
            pDevContext->driverMode = DM_SLIDER_SWITCH;
            SendVendorCommand(hDevice, HIDFX2_SET_7SEGMENT_DISPLAY, SEGMENT_DISPLAY_3);
            break;
        case SWITCHPACK_SELECTION_FOR_MODE4:
            pDevContext->driverMode = DM_SLIDER_SWITCH_AND_LED;
            SendVendorCommand(hDevice, HIDFX2_SET_7SEGMENT_DISPLAY, SEGMENT_DISPLAY_4);
            break;
        case SWITCHPACK_SELECTION_FOR_MODE5:
            pDevContext->driverMode = DM_LED_ONLY;
            SendVendorCommand(hDevice, HIDFX2_SET_7SEGMENT_DISPLAY, SEGMENT_DISPLAY_5);
            break;
        default:
            pDevContext->driverMode = DM_BUTTON;
            SendVendorCommand(hDevice, HIDFX2_SET_7SEGMENT_DISPLAY, SEGMENT_DISPLAY_1);
            break;
        }

        pDevContext->bCurrentSwitchState = bSwitchState;

        // Start the target.  This will start the continuous reader
        status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(pDevContext->hInterruptPipe));
        if (NT_SUCCESS(status))
        {
            pDevContext->fIsPowerUpSwitchState = TRUE;
        }
    }
    else
    {
        TraceErr(DBG_PNP, "(%!FUNC!) Failed to get current switch state, status: %!STATUS!\n", status);
    }

    TraceVerbose(DBG_PNP, "(%!FUNC!) Exit, status: %!STATUS!\n", status);
    return status;
}



/*
This routine undoes anything done in EvtDeviceD0Entry.  It is called  whenever the device leaves the D0 state, which happens when the device is
stopped, when it is removed, and when it is powered off.

The device is still in D0 when this callback is invoked, which means that  the driver can still touch hardware in this routine.

EvtDeviceD0Exit event callback must perform any operations that are necessary before the specified device is moved out of the D0 state.  
If the  driver needs to save hardware state before the device is powered down, then that should be done here.

This function runs at PASSIVE_LEVEL, though it is generally not paged.  A  driver can optionally make this function pageable if DO_POWER_PAGABLE is set.

Even if DO_POWER_PAGABLE isn't set, this function still runs at PASSIVE_LEVEL.
In this case, though, the function absolutely must not do anything that will cause a page fault.
*/
NTSTATUS HidFx2EvtDeviceD0Exit(_In_ WDFDEVICE hDevice, _In_ WDF_POWER_DEVICE_STATE targetState)
{
    PDEVICE_EXTENSION         pDevContext;

    PAGED_CODE();

    TraceVerbose(DBG_PNP, "(%!FUNC!) Enter- moving to %S\n", DbgDevicePowerString(targetState));

    pDevContext = GetDeviceContext(hDevice);

    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(pDevContext->hInterruptPipe), WdfIoTargetCancelSentIo);

    TraceVerbose(DBG_PNP, "(%!FUNC!) Exit\n");

    return STATUS_SUCCESS;
}



PCWSTR DbgDevicePowerString(_In_ WDF_POWER_DEVICE_STATE powerState)
{
    switch (powerState)
    {
    case WdfPowerDeviceInvalid:
        return L"WdfPowerDeviceInvalid";
    case WdfPowerDeviceD0:
        return L"WdfPowerDeviceD0";
    case PowerDeviceD1:
        return L"WdfPowerDeviceD1";
    case WdfPowerDeviceD2:
        return L"WdfPowerDeviceD2";
    case WdfPowerDeviceD3:
        return L"WdfPowerDeviceD3";
    case WdfPowerDeviceD3Final:
        return L"WdfPowerDeviceD3Final";
    case WdfPowerDevicePrepareForHibernation:
        return L"WdfPowerDevicePrepareForHibernation";
    case WdfPowerDeviceMaximum:
        return L"PowerDeviceMaximum";
    default:
        return L"UnKnown Device Power State";
    }
}

