/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Device.c

Abstract:

    USB device driver for OSR USB-FX2 Learning Kit

Environment:

    Kernel mode only

--*/

#include <osrusbfx2.h>
#include "rawpdo.h"

#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, OsrFxEvtDeviceAdd)
#pragma alloc_text(PAGE, OsrFxEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, OsrFxEvtDeviceD0Exit)
#endif


NTSTATUS
OsrFxEvtDeviceAdd(
    IN WDFDRIVER        Driver,
    IN PWDFDEVICE_INIT  DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device. All the software resources
    should be allocated in this callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    WDF_PNPPOWER_EVENT_CALLBACKS        pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES               attributes;
    NTSTATUS                            status;
    WDFDEVICE                           device;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,"--> OsrFxEvtDeviceAdd routine\n");

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    //
    // For usb devices, PrepareHardware callback is the to place select the
    // interface and configure the device.
    //
    pnpPowerCallbacks.EvtDevicePrepareHardware = OsrFxEvtDevicePrepareHardware;

    //
    // These two callbacks start and stop the WDFUSBPIPE continuous reader
    // as we go in and out of the D0-working state.
    //

    pnpPowerCallbacks.EvtDeviceD0Entry = OsrFxEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit  = OsrFxEvtDeviceD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    OsrFxInitChildList(DeviceInit);

    //
    // Now specify the size of device extension where we track per device
    // context.DeviceInit is completely initialized. So call the framework
    // to create the device and attach it to the lower stack.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceCreate failed with Status code %!STATUS!\n", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "OsrFxEvtDeviceAdd  - ends\n");

    return status;
}

NTSTATUS
OsrFxEvtDevicePrepareHardware(
    IN WDFDEVICE    Device,
    IN WDFCMRESLIST ResourceList,
    IN WDFCMRESLIST ResourceListTranslated
    )
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.  In the case of a USB device, this involves
    reading descriptors and selecting interfaces.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS                            status, tempStatus;
    PDEVICE_CONTEXT                     pDeviceContext;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "--> EvtDevicePrepareHardware\n");

    pDeviceContext = GetDeviceContext(Device);

    //
    // Create a USB device handle so that we can communicate with the
    // underlying USB stack. The WDFUSBDEVICE handle is used to query,
    // configure, and manage all aspects of the USB device.
    // These aspects include device properties, bus properties,
    // and I/O creation and synchronization. We only create device the first
    // the PrepareHardware is called. If the device is restarted by pnp manager
    // for resource rebalance, we will use the same device handle but then select
    // the interfaces again because the USB stack could reconfigure the device on
    // restart.
    //
    if (pDeviceContext->UsbDevice == NULL) {
        WDF_USB_DEVICE_CREATE_CONFIG config;

        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config,
                                   USBD_CLIENT_CONTRACT_VERSION_602);

        status = WdfUsbTargetDeviceCreateWithParameters(Device,
                                               &config,
                                               WDF_NO_OBJECT_ATTRIBUTES,
                                               &pDeviceContext->UsbDevice);

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                 "WdfUsbTargetDeviceCreateWithParameters failed with Status code %!STATUS!\n", status);
            return status;
        }
    }

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE( &configParams);

    status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &configParams);
    if(!NT_SUCCESS(status)) {
        WDF_USB_DEVICE_INFORMATION  deviceInfo;

        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                        "WdfUsbTargetDeviceSelectConfig failed %!STATUS! \n",
                        status);
        //
        // detect if we are connected to a 1.1 USB port 
        //
        WDF_USB_DEVICE_INFORMATION_INIT(&deviceInfo);
        tempStatus = WdfUsbTargetDeviceRetrieveInformation(pDeviceContext->UsbDevice, &deviceInfo);

        if (NT_SUCCESS(tempStatus)) {
            //
            // Since the Osr USB fx2 device is capable of working at high speed, the only reason 
            // the device would not be working at high speed is if the port doesn't 
            // support it. If the port doesn't support high speed it is a 1.1 port
            //
            if ((deviceInfo.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED) == 0) {
                TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                                " On a 1.1 USB port on Windows Vista" 
                                " this is expected as the OSR USB Fx2 board's Interrupt EndPoint descriptor" 
                                " doesn't conform to the USB specification. Windows Vista detects this and"
                                " returns an error. \n"
                                );
            }
        }

        return status;
    }

    pDeviceContext->UsbInterface =
                configParams.Types.SingleInterface.ConfiguredUsbInterface;

    status = OsrFxConfigContReaderForInterruptEndPoint(pDeviceContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "<-- EvtDevicePrepareHardware\n");

    return status;
}


NTSTATUS
OsrFxEvtDeviceD0Entry(
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
    PDEVICE_CONTEXT         pDeviceContext;
    NTSTATUS                status;

    pDeviceContext = GetDeviceContext(Device);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER,
                "-->OsrFxEvtEvtDeviceD0Entry - coming from %s\n",
                DbgDevicePowerString(PreviousState));

    status = WdfIoTargetStart(WdfUsbTargetDeviceGetIoTarget(pDeviceContext->UsbDevice));

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER, "<--OsrFxEvtEvtDeviceD0Entry\n");

    return status;
}


NTSTATUS
OsrFxEvtDeviceD0Exit(
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
    PDEVICE_CONTEXT         pDeviceContext;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER,
          "-->OsrFxEvtDeviceD0Exit - moving to %s\n",
          DbgDevicePowerString(TargetState));

    pDeviceContext = GetDeviceContext(Device);

    WdfIoTargetStop(WdfUsbTargetDeviceGetIoTarget(pDeviceContext->UsbDevice),
                    WdfIoTargetCancelSentIo);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_POWER, "<--OsrFxEvtDeviceD0Exit\n");

    return STATUS_SUCCESS;
}


_IRQL_requires_(PASSIVE_LEVEL)
PCHAR
DbgDevicePowerString(
    _In_ WDF_POWER_DEVICE_STATE Type
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


