/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    usb.c

Abstract:

    Code for handling USB related requests

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include <hidusbfx2.h>

#if defined(EVENT_TRACING)
#include "usb.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HidFx2EvtDevicePrepareHardware)
#pragma alloc_text(PAGE, HidFx2EvtDeviceD0Exit)
#pragma alloc_text(PAGE, HidFx2ConfigContReaderForInterruptEndPoint)
#pragma alloc_text(PAGE, HidFx2ValidateConfigurationDescriptor)
#endif

NTSTATUS
HidFx2EvtDevicePrepareHardware(
    IN WDFDEVICE    Device,
    IN WDFCMRESLIST ResourceList,
    IN WDFCMRESLIST ResourceListTranslated
    )
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.  In the case of a USB device, this involves
    reading and selecting descriptors.

Arguments:

    Device - handle to a device

    ResourceList - A handle to a framework resource-list object that
    identifies the raw hardware resourcest

    ResourceListTranslated - A handle to a framework resource-list object
    that identifies the translated hardware resources

Return Value:

    NT status value

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION                   devContext = NULL;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    WDF_OBJECT_ATTRIBUTES               attributes;
    PUSB_DEVICE_DESCRIPTOR              usbDeviceDescriptor = NULL;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
        "HidFx2EvtDevicePrepareHardware Enter\n");

    devContext = GetDeviceContext(Device);
   
    //
    // Create a WDFUSBDEVICE object. WdfUsbTargetDeviceCreate obtains the
    // USB device descriptor and the first USB configuration descriptor from
    // the device and stores them. It also creates a framework USB interface
    // object for each interface in the device's first configuration.
    //
    // The parent of each USB device object is the driver's framework driver
    // object. The driver cannot change this parent, and the ParentObject
    // member or the WDF_OBJECT_ATTRIBUTES structure must be NULL.
    //
    // We only create device the first time PrepareHardware is called. If 
    // the device is restarted by pnp manager for resource rebalance, we 
    // will use the same device handle but then select the interfaces again
    // because the USB stack could reconfigure the device on restart.
    //
    if (devContext->UsbDevice == NULL) {
        status = WdfUsbTargetDeviceCreate(Device,
                                          WDF_NO_OBJECT_ATTRIBUTES,
                                          &devContext->UsbDevice);
  
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                "WdfUsbTargetDeviceCreate failed 0x%x\n", status);
            return status;
        }

        //
        // TODO: If you are fetching configuration descriptor from device for
        // selecting a configuration or to parse other descriptors, call 
        // HidFx2ValidateConfigurationDescriptor
        // to do basic validation on the descriptors before you access them.
        //

    }

    //
    // Select a device configuration by using a
    // WDF_USB_DEVICE_SELECT_CONFIG_PARAMS structure to specify USB
    // descriptors, a URB, or handles to framework USB interface objects.
    //
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE( &configParams);

    status = WdfUsbTargetDeviceSelectConfig(devContext->UsbDevice,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &configParams);
    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfUsbTargetDeviceSelectConfig failed %!STATUS!\n",
            status);
        return status;
    }

    devContext->UsbInterface =
                configParams.Types.SingleInterface.ConfiguredUsbInterface;

    //
    // Get the device descriptor and store it in device context
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;
    status = WdfMemoryCreate(
                             &attributes,
                             NonPagedPool,
                             0,
                             sizeof(USB_DEVICE_DESCRIPTOR),
                             &devContext->DeviceDescriptor,
                             &usbDeviceDescriptor
                             );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfMemoryCreate for Device Descriptor failed %!STATUS!\n",
            status);
        return status;
    }

    WdfUsbTargetDeviceGetDeviceDescriptor(
          devContext->UsbDevice,
          usbDeviceDescriptor
          );

    //
    // Get the Interrupt pipe. There are other endpoints but we are only
    // interested in interrupt endpoint since our HID data comes from that
    // endpoint. Another way to get the interrupt endpoint is by enumerating
    // through all the pipes in a loop and looking for pipe of Interrupt type.
    //
    devContext->InterruptPipe = WdfUsbInterfaceGetConfiguredPipe(
                                                  devContext->UsbInterface,
                                                  INTERRUPT_ENDPOINT_INDEX,
                                                  NULL);// pipeInfo

    if (NULL == devContext->InterruptPipe) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "Failed to get interrupt pipe info\n");
        status = STATUS_INVALID_DEVICE_STATE;
        return status;
    }

    //
    // Tell the framework that it's okay to read less than
    // MaximumPacketSize
    //
    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(devContext->InterruptPipe);

    //
    //configure continuous reader
    //
    status = HidFx2ConfigContReaderForInterruptEndPoint(devContext);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
        "HidFx2EvtDevicePrepareHardware Exit, Status:0x%x\n", status);

    return status;
}


NTSTATUS
HidFx2ConfigContReaderForInterruptEndPoint(
    PDEVICE_EXTENSION DeviceContext
    )
/*++

Routine Description:

    This routine configures a continuous reader on the
    interrupt endpoint. It's called from the PrepareHarware event.

Arguments:

    DeviceContext - Pointer to device context structure

Return Value:

    NT status value

--*/
{
    WDF_USB_CONTINUOUS_READER_CONFIG contReaderConfig;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
        "HidFx2ConfigContReaderForInterruptEndPoint Enter\n");

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&contReaderConfig,
                                          HidFx2EvtUsbInterruptPipeReadComplete,
                                          DeviceContext,    // Context
                                          sizeof(UCHAR));   // TransferLength
    //
    // Reader requests are not posted to the target automatically.
    // Driver must explictly call WdfIoTargetStart to kick start the
    // reader.  In this sample, it's done in D0Entry.
    // By defaut, framework queues two requests to the target
    // endpoint. Driver can configure up to 10 requests with CONFIG macro.
    //
    status = WdfUsbTargetPipeConfigContinuousReader(DeviceContext->InterruptPipe,
                                                    &contReaderConfig);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                    "HidFx2ConfigContReaderForInterruptEndPoint failed %x\n",
                    status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
        "HidFx2ConfigContReaderForInterruptEndPoint Exit, status:0x%x\n", status);

    return status;
}


VOID
HidFx2EvtUsbInterruptPipeReadComplete(
    WDFUSBPIPE  Pipe,
    WDFMEMORY   Buffer,
    size_t      NumBytesTransferred,
    WDFCONTEXT  Context
    )
/*++

Routine Description:

    This the completion routine of the continuous reader. This can
    called concurrently on multiprocessor system if there are
    more than one readers configured. So make sure to protect
    access to global resources.

Arguments:

    Pipe - Handle to WDF USB pipe object

    Buffer - This buffer is freed when this call returns.
             If the driver wants to delay processing of the buffer, it
             can take an additional referrence.

    NumBytesTransferred - number of bytes of data that are in the read buffer.

    Context - Provided in the WDF_USB_CONTINUOUS_READER_CONFIG_INIT macro

Return Value:

    NT status value

--*/
{
    PDEVICE_EXTENSION  devContext = Context;
    UCHAR              toggledSwitch = 0;
    PUCHAR             switchState = NULL;
    UCHAR              currentSwitchState = 0;
    UCHAR              previousSwitchState = 0;

    UNREFERENCED_PARAMETER(NumBytesTransferred);
    UNREFERENCED_PARAMETER(Pipe);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
        "HidFx2EvtUsbInterruptPipeReadComplete Enter\n");

    //
    // Interrupt endpoints sends switch state when first started
    // or when resuming from suspend. We need to ignore that data since
    // user did not change the switch state.
    //
    if (devContext->IsPowerUpSwitchState) {
        devContext->IsPowerUpSwitchState = FALSE;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
            "Dropping interrupt message since received during powerup/resume\n");
        return;
    }


    //
    // Make sure that there is data in the read packet.  Depending on the device
    // specification, it is possible for it to return a 0 length read in
    // certain conditions.
    //

    if (NumBytesTransferred == 0) {
        TraceEvents(TRACE_LEVEL_WARNING, DBG_INIT,
                    "HidFx2EvtUsbInterruptPipeReadComplete Zero length read "
                    "occured on the Interrupt Pipe's Continuous Reader\n"
                    );
        return;
    }
    
    switchState = WdfMemoryGetBuffer(Buffer, NULL);

    currentSwitchState = *switchState;
    previousSwitchState = devContext->CurrentSwitchState;

    //
    // we want to know which switch got toggled from 0 to 1
    // Since the device returns the state of all the swicthes and not just the
    // one that got toggled, we need to store previous state and xor
    // it with current state to know whcih one swicth got toggled.
    // Further, the toggle is considered "on" only when it changes from 0 to 1
    // (and not when it changes from 1 to 0).
    //
    toggledSwitch = (previousSwitchState ^ currentSwitchState) & currentSwitchState;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                "HidFx2EvtUsbInterruptPipeReadComplete SwitchState %x, "
                "prevSwitch:0x%x, x0R:0x%x\n",
                currentSwitchState,
                previousSwitchState,
                toggledSwitch
                );

    //
    // Store switch state in device context
    //
    devContext->CurrentSwitchState = *switchState;
    //if (toggledSwitch != 0) {
        devContext->LatestToggledSwitch = toggledSwitch;
    //}

    //
    // Complete pending Read requests if there is at least one switch toggled
    // to on position.
    //
    if (toggledSwitch != 0) {
        BOOLEAN inTimerQueue;

        //
        // Debounce the switchpack. A simple logic is used for debouncing.
        // A timer is started for 10 ms everytime there is a switch toggled on.
        // If within 10 ms same or another switch gets toggled, the timer gets
        // reset for another 10 ms. The HID read request is completed in timer
        // function if there is still a switch in toggled-on state. Note that
        // debouncing happens at the whole switch pack level (not individual
        // switches) which means if two different switches are toggled-on within
        // 10 ms only one of them (later one in this case) will get accepted and
        // sent to hidclass driver
        //
        inTimerQueue = WdfTimerStart(
            devContext->DebounceTimer,
            WDF_REL_TIMEOUT_IN_MS(SWICTHPACK_DEBOUNCE_TIME_IN_MS)
            );

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
            "Debounce Timer started with timeout of %d ms"
            " (TimerReturnValue:%d)\n",
            SWICTHPACK_DEBOUNCE_TIME_IN_MS, inTimerQueue);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
        "HidFx2EvtUsbInterruptPipeReadComplete Exit\n");
}


VOID
HidFx2CompleteReadReport(
    WDFDEVICE Device
    )
/*++

Routine Description

    This method handles the completion of the pended request for the
    IOCTL_HID_READ_REPORT

Arguments:

    Device - Handle to a framework device.

Return Value:

    None.

--*/
{
    NTSTATUS             status = STATUS_SUCCESS;
    WDFREQUEST           request;
    PDEVICE_EXTENSION    pDevContext = NULL;
    size_t               bytesReturned = 0;
#ifndef USE_ALTERNATE_HID_REPORT_DESCRIPTOR
    UCHAR                toggledSwitch = 0;
#endif // USE_ALTERNATE_HID_REPORT_DESCRIPTOR
    ULONG                bytesToCopy = 0;
    PHIDFX2_INPUT_REPORT inputReport = NULL;

    pDevContext = GetDeviceContext(Device);

    //
    // Check if there are any pending requests in the Interrupt Message Queue.
    // If a request is found then complete the pending request.
    //
    status = WdfIoQueueRetrieveNextRequest(pDevContext->InterruptMsgQueue, &request);

    if (NT_SUCCESS(status)) {
        //
        // IOCTL_HID_READ_REPORT is METHOD_NEITHER so WdfRequestRetrieveOutputBuffer
        // will correctly retrieve buffer from Irp->UserBuffer. Remember that
        // HIDCLASS provides the buffer in the Irp->UserBuffer field
        // irrespective of the ioctl buffer type. However, framework is very
        // strict about type checking. You cannot get Irp->UserBuffer by using
        // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
        // internal ioctl.
        //
        bytesToCopy = sizeof(HIDFX2_INPUT_REPORT);
        status = WdfRequestRetrieveOutputBuffer(request,
                                                bytesToCopy,
                                                &inputReport,
                                                &bytesReturned);// BufferLength

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "WdfRequestRetrieveOutputBuffer failed with status: 0x%x\n", status);
        } else {

#ifndef USE_ALTERNATE_HID_REPORT_DESCRIPTOR
            //
            // Map switch pack state. The lower 7 bits of switch pack
            // state are mapped to usages in consumer control collection
            // while the highest one bit is mapped to sleep usage in system
            // control collection
            //
            toggledSwitch = pDevContext->LatestToggledSwitch;

			if (toggledSwitch & CONSUMER_CONTROL_BUTTONS_BIT_MASK) {
                //
                //these are consumer control buttons
                //
                TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                    "Consumer control SwitchState: 0x%x\n", toggledSwitch);

                inputReport->ReportId = CONSUMER_CONTROL_REPORT_ID;
                inputReport->SwitchStateAsByte = toggledSwitch;
                bytesReturned = bytesToCopy;
            }
            else if (toggledSwitch & SYSTEM_CONTROL_BUTTONS_BIT_MASK) {
                //
                // these are system control buttons
                //
                TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                    "System Control SwitchState: 0x%x\n", toggledSwitch);

                inputReport->ReportId = SYSTEM_CONTROL_REPORT_ID;
                inputReport->SwitchStateAsByte = toggledSwitch;
                bytesReturned = bytesToCopy;
            }
            else {
                //
                // We can't be here since we already rejected the switch
                // state with no swicthes turned on
                //
                ASSERT(FALSE);
            }
#else
            //
            // Using vendor collection reports instead of HID collections that integrate
            // into consumer and system control
            //
            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL,
                "Vendor SwitchState: 0x%x\n", pDevContext->CurrentSwitchState);

            inputReport->ReportId = DIP_SWITCHES_REPORT_ID;
            inputReport->SwitchStateAsByte = pDevContext->CurrentSwitchState;
            bytesReturned = bytesToCopy;

#endif // USE_ALTERNATE_HID_REPORT_DESCRIPTOR

        }

        WdfRequestCompleteWithInformation(request, status, bytesReturned);

    } else if (status != STATUS_NO_MORE_ENTRIES) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
            "WdfIoQueueRetrieveNextRequest status %08x\n", status);
    }

    return;
}


NTSTATUS
HidFx2EvtDeviceD0Entry(
    IN  WDFDEVICE Device,
    IN  WDF_POWER_DEVICE_STATE PreviousState
    )
/*++

Routine Description:

    EvtDeviceD0Entry event callback must perform any operations that are
    necessary before the specified device is used.  It will be called every
    time the hardware needs to be (re-)initialized.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

    This function runs at PASSIVE_LEVEL, even though it is not paged.  A
    driver can optionally make this function pageable if DO_POWER_PAGABLE
    is set.  Even if DO_POWER_PAGABLE isn't set, this function still runs
    at PASSIVE_LEVEL.  In this case, though, the function absolutely must
    not do anything that will cause a page fault.

Arguments:

    Device - Handle to a framework device object.

    PreviousState - Device power state which the device was in most recently.
        If the device is being newly started, this will be
        PowerDeviceUnspecified.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   devContext = NULL;
    NTSTATUS            status = STATUS_SUCCESS;
    UCHAR               switchState = 0;

    devContext = GetDeviceContext(Device);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP,
        "HidFx2EvtDeviceD0Entry Enter - coming from %s\n",
                DbgDevicePowerString(PreviousState));

    //
    // Retrieve the current switch state and store it in device context
    //
    status = HidFx2GetSwitchState(Device, &switchState);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "Failed to get current swicth state, status: 0x%x\n", status);
        return status;
    }

    devContext->CurrentSwitchState = switchState;

    //
    // Start the target. This will start the continuous reader
    //
    status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(devContext->InterruptPipe));
    if (NT_SUCCESS(status)) {
        devContext->IsPowerUpSwitchState = TRUE;
    }

    TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
        "HidFx2EvtDeviceD0Entry Exit, status: 0x%x\n", status);

    return status;
}


NTSTATUS
HidFx2EvtDeviceD0Exit(
    IN  WDFDEVICE Device,
    IN  WDF_POWER_DEVICE_STATE TargetState
    )
/*++

Routine Description:

    This routine undoes anything done in EvtDeviceD0Entry.  It is called
    whenever the device leaves the D0 state, which happens when the device is
    stopped, when it is removed, and when it is powered off.

    The device is still in D0 when this callback is invoked, which means that
    the driver can still touch hardware in this routine.


    EvtDeviceD0Exit event callback must perform any operations that are
    necessary before the specified device is moved out of the D0 state.  If the
    driver needs to save hardware state before the device is powered down, then
    that should be done here.

    This function runs at PASSIVE_LEVEL, though it is generally not paged.  A
    driver can optionally make this function pageable if DO_POWER_PAGABLE is set.

    Even if DO_POWER_PAGABLE isn't set, this function still runs at
    PASSIVE_LEVEL.  In this case, though, the function absolutely must not do
    anything that will cause a page fault.

Arguments:

    Device - Handle to a framework device object.

    TargetState - Device power state which the device will be put in once this
        callback is complete.

Return Value:

    Success implies that the device can be used.  Failure will result in the
    device stack being torn down.

--*/
{
    PDEVICE_EXTENSION         devContext;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP,
        "HidFx2EvtDeviceD0Exit Enter- moving to %s\n",
          DbgDevicePowerString(TargetState));

    devContext = GetDeviceContext(Device);

    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(
        devContext->InterruptPipe), WdfIoTargetCancelSentIo);

    TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
        "HidFx2EvtDeviceD0Exit Exit\n");

    return STATUS_SUCCESS;
}


NTSTATUS
HidFx2GetSwitchState(
    IN WDFDEVICE Device,
    OUT PUCHAR SwitchState
    )
/*++

Routine Description:

    This function gets the swicth state of teh USB device

Arguments:

    Device - Handle to a framework device object.

    SwitchState - Pointer to a variable that receives the switch state

Return Value:

    Success implies that the device can be used.  Failure will result in the
    device stack being torn down.

--*/
{
    PDEVICE_EXTENSION            devContext = NULL;
    NTSTATUS                     status = STATUS_SUCCESS;
    WDF_MEMORY_DESCRIPTOR        memDesc;
    WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
    ULONG                        bytesTransferred = 0;

    devContext = GetDeviceContext(Device);

    //
    // set the segment state on the USB device
    //
    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestDeviceToHost,
                                        BmRequestToDevice,
                                        HIDFX2_READ_SWITCH_STATE, // Request
                                        0, // Value
                                        0); // Index

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
                                    SwitchState,
                                    sizeof(UCHAR));

    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                                devContext->UsbDevice,
                                                NULL, // Optional WDFREQUEST
                                                NULL, // PWDF_REQUEST_SEND_OPTIONS
                                                &controlSetupPacket,
                                                &memDesc,
                                                &bytesTransferred
                                                );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
            "GetSwitchState: Failed to read switch state - 0x%x \n", status);

    } else {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
            "GetSwitchState: Switch state is 0x%x\n", *SwitchState);
    }

    return status;
}


VOID
HidFx2EvtTimerFunction(
    IN WDFTIMER  Timer
    )
/*++

Routine Description:

    This function gets called when the timeout period of debounce timer elapses.

Arguments:

    Timer - Handle to a framework timer object.

Return Value:

    none

--*/
{
#ifndef USE_ALTERNATE_HID_REPORT_DESCRIPTOR
    PDEVICE_EXTENSION  devContext =
        GetDeviceContext(WdfTimerGetParentObject(Timer));

    //
    // Complete the request if there is a swicthed in toggled-on position
    //
    if (devContext->LatestToggledSwitch != 0) {
        HidFx2CompleteReadReport(WdfTimerGetParentObject(Timer));
    }

#else

    //
    // Always complete the read request for the vendor collection
    // input report.
    //
    HidFx2CompleteReadReport(WdfTimerGetParentObject(Timer));

#endif // USE_ALTERNATE_HID_REPORT_DESCRIPTOR
}

USBD_STATUS
HidFx2ValidateConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    IN ULONG BufferLength,
    _Inout_  PUCHAR *Offset
    )
/*++

Routine Description:

    Validates a USB Configuration Descriptor

Parameters:

    ConfigDesc: Pointer to the entire USB Configuration descriptor returned by the device

    BufferLength: Known size of buffer pointed to by ConfigDesc (Not wTotalLength)

    Offset: if the USBD_STATUS returned is not USBD_STATUS_SUCCESS, offet will
        be set to the address within the ConfigDesc buffer where the failure occured.

Return Value:

    USBD_STATUS
    Success implies the configuration descriptor is valid.

--*/
{


    USBD_STATUS status = USBD_STATUS_SUCCESS;
    USHORT ValidationLevel = 3;

    PAGED_CODE();

    //
    // Call USBD_ValidateConfigurationDescriptor to validate the descriptors which are present in this supplied configuration descriptor.
    // USBD_ValidateConfigurationDescriptor validates that all descriptors are completely contained within the configuration descriptor buffer.
    // It also checks for interface numbers, number of endpoints in an interface etc.
    // Please refer to msdn documentation for this function for more information.
    //  
   
    status = USBD_ValidateConfigurationDescriptor( ConfigDesc, BufferLength , ValidationLevel , Offset , POOL_TAG );
    if (!(NT_SUCCESS (status)) ){
        return status;
    }

    //
    // TODO: You should validate the correctness of other descriptors which are not taken care by USBD_ValidateConfigurationDescriptor
    // Check that all such descriptors have size >= sizeof(the descriptor they point to) 
    // Check for any association between them if required 
    // 
   
    return status;
}


PCHAR
DbgDevicePowerString(
    IN WDF_POWER_DEVICE_STATE Type
    )
/*++

Updated Routine Description:
    DbgDevicePowerString does not change in this stage of the function driver.

--*/
{
    switch (Type)
    {
    case WdfPowerDeviceInvalid:
        return "WdfPowerDeviceInvalid";
    case WdfPowerDeviceD0:
        return "WdfPowerDeviceD0";
    case WdfPowerDeviceD1:
        return "WdfPowerDeviceD1";
    case WdfPowerDeviceD2:
        return "WdfPowerDeviceD2";
    case WdfPowerDeviceD3:
        return "WdfPowerDeviceD3";
    case WdfPowerDeviceD3Final:
        return "WdfPowerDeviceD3Final";
    case WdfPowerDevicePrepareForHibernation:
        return "WdfPowerDevicePrepareForHibernation";
    case WdfPowerDeviceMaximum:
        return "WdfPowerDeviceMaximum";
    default:
        return "UnKnown Device Power State";
    }
}
