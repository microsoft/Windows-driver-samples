/*++

Module Name:

    Device.c - Device handling events.

Abstract:

   This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, EvtCreateDevice)
#pragma alloc_text (PAGE, EvtPrepareHardware)
#pragma alloc_text (PAGE, EvtDeviceD0Entry)
#pragma alloc_text (PAGE, EvtReleaseHardware)
#endif

NTSTATUS
EvtCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. Don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_DEVICE);

    PAGED_CODE();

    NTSTATUS status;
    WDFDEVICE device;
    PDEVICE_CONTEXT deviceContext;
    UCMTCPCI_DEVICE_CONFIG config;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = EvtPrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = EvtReleaseHardware;
    pnpPowerCallbacks.EvtDeviceD0Entry = EvtDeviceD0Entry;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    status = UcmTcpciDeviceInitInitialize(DeviceInit);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "[PWDFDEVICE_INIT: 0x%p] UcmTcpciDeviceInitInitialize failed - %!STATUS!",
            DeviceInit, status);
        goto Exit;
    }

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "[PWDFDEVICE_INIT: 0x%p] WdfDeviceCreate failed - %!STATUS!", DeviceInit, status);
        goto Exit;
    }

    deviceContext = DeviceGetContext(device);

    // Save the device in the context so we can access it later.
    deviceContext->Device = device;

    // Initialize platform-level device reset.
    deviceContext->ResetAttempts = 0;

    RtlZeroMemory(&deviceContext->ResetInterface, sizeof(deviceContext->ResetInterface));
    deviceContext->ResetInterface.Size = sizeof(deviceContext->ResetInterface);
    deviceContext->ResetInterface.Version = 1;

    status = WdfFdoQueryForInterface(
        deviceContext->Device,
        &GUID_DEVICE_RESET_INTERFACE_STANDARD,
        (PINTERFACE)&deviceContext->ResetInterface,
        sizeof(deviceContext->ResetInterface),
        1,
        NULL);

    // The reset interface may not exist on certain environments.
    // In this case, we will fall back to using a different reset method.
    // Zero the reset interface and ignore the error.
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "[WDFDEVICE: 0x%p] WdfFdoQueryForInterface for GUID_DEVICE_RESET_INTERFACE_STANDARD failed. Status: %!STATUS!",
            device, status);
        RtlZeroMemory(&deviceContext->ResetInterface, sizeof(deviceContext->ResetInterface));
        status = STATUS_SUCCESS;
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "[WDFDEVICE: 0x%p] Successfully initialized platform-level device reset.", deviceContext->Device);
    }

    // Register our device with UcmTcpciCx.
    UCMTCPCI_DEVICE_CONFIG_INIT(&config);
    status = UcmTcpciDeviceInitialize(device, &config);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "[WDFDEVICE: 0x%p] UcmTcpciDeviceInitialize failed - %!STATUS!",
            device, status);
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_DEVICE);
    return status;
}

NTSTATUS
EvtPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
)
/*++

Routine Description:

    A driver's EvtDevicePrepareHardware event callback function performs any operations
    that are needed to make a device accessible to the driver.

Arguments:

    Device - A handle to a framework device object.

    ResourcesRaw - A handle to a framework resource-list object that identifies the raw hardware
    resources that the Plug and Play manager has assigned to the device.

    ResourcesTranslated - A handle to a framework resource-list object that identifies the
    translated hardware resources that the Plug and Play manager has assigned to the device.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_DEVICE);

    PAGED_CODE();

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;

    deviceContext = DeviceGetContext(Device);

    //// Initialize the I2C communication channel to read from/write to the hardware.
    status = I2CInitialize(deviceContext, ResourcesRaw, ResourcesTranslated);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = I2COpen(deviceContext);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    return status;
}

NTSTATUS EvtDeviceD0Entry(
    _In_ WDFDEVICE              Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
)
{
    TRACE_FUNC_ENTRY(TRACE_DEVICE);

    UNREFERENCED_PARAMETER(PreviousState);

    PAGED_CODE();

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    UCMTCPCIPORTCONTROLLER portController = WDF_NO_HANDLE;
    UCMTCPCI_PORT_CONTROLLER_CONFIG config;
    UCMTCPCI_PORT_CONTROLLER_IDENTIFICATION ident;
    UCMTCPCI_PORT_CONTROLLER_CAPABILITIES capabilities;

    deviceContext = DeviceGetContext(Device);

    UCMTCPCI_PORT_CONTROLLER_IDENTIFICATION_INIT(&ident);
    UCMTCPCI_PORT_CONTROLLER_CAPABILITIES_INIT(&capabilities);

    // Read device identification and capabilities from the registers.
    REGISTER_ITEM items[] = {
        GEN_REGISTER_ITEM(VENDOR_ID,                    ident.VendorId),
        GEN_REGISTER_ITEM(PRODUCT_ID,                   ident.ProductId),
        GEN_REGISTER_ITEM(DEVICE_ID,                    ident.DeviceId),
        GEN_REGISTER_ITEM(USBTYPEC_REV,                 ident.TypeCRevisionInBcd),
        GEN_REGISTER_ITEM(USBPD_REV_VER,                ident.PDRevisionAndVersionInBcd),
        GEN_REGISTER_ITEM(PD_INTERFACE_REV,             ident.PDInterfaceRevisionAndVersionInBcd),
        GEN_REGISTER_ITEM(DEVICE_CAPABILITIES_1,        capabilities.DeviceCapabilities1),
        GEN_REGISTER_ITEM(DEVICE_CAPABILITIES_2,        capabilities.DeviceCapabilities2),
        GEN_REGISTER_ITEM(STANDARD_INPUT_CAPABILITIES,  capabilities.StandardInputCapabilities),
        GEN_REGISTER_ITEM(STANDARD_OUTPUT_CAPABILITIES, capabilities.StandardOutputCapabilities),
    };

    status = I2CReadSynchronouslyMultiple(deviceContext,
        I2CRequestSourceClient,
        items,
        _countof(items));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }


    capabilities.IsPowerDeliveryCapable = TRUE;

    UCMTCPCI_PORT_CONTROLLER_CONFIG_INIT(&config, &ident, &capabilities);

    // Create a UCMTCPCIPORTCONTROLLER framework object.
    status = UcmTcpciPortControllerCreate(Device, &config, WDF_NO_OBJECT_ATTRIBUTES, &portController);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "[WDFDEVICE: 0x%p] UcmTcpciPortControllerCreate "
            "failed - %!STATUS!", Device, status);
        goto Exit;
    }

    // Save the UCMTCPCIPORTCONTROLLER in our device context.
    deviceContext = DeviceGetContext(Device);
    deviceContext->PortController = portController;

    // Set the hardware request queue for our device.
    status = HardwareRequestQueueInitialize(Device);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    // Direct UcmTcpciCx to start the port controller.
    // At this point, UcmTcpciCx will assume control of USB Type-C and Power Delivery.
    // After the port controller is started, UcmTcpciCx may start putting requests into the
    // hardware request queue.
    status = UcmTcpciPortControllerStart(portController);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "[UCMTCPCIPORTCONTROLLER: 0x%p]"
            "UcmTcpciPortControllerStart failed - %!STATUS!", portController, status);
        goto Exit;
    }

Exit:
    if (!NT_SUCCESS(status) && (portController != WDF_NO_HANDLE))
    {
        WdfObjectDelete(portController);
        deviceContext->PortController = WDF_NO_HANDLE;
    }

    TRACE_FUNC_EXIT(TRACE_DEVICE);
    return status;
}

NTSTATUS
EvtReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
)
/*++

Routine Description:

    A driver's EvtDeviceReleaseHardware event callback function performs operations
    that are needed when a device is no longer accessible.

Arguments:

    Device - A handle to a framework device object.

    ResourcesTranslated - A handle to a resource list object that identifies the translated
    hardware resources that the Plug and Play manager has assigned to the device.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_DEVICE);

    UNREFERENCED_PARAMETER(ResourcesTranslated);
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT deviceContext;

    deviceContext = DeviceGetContext(Device);

    if (deviceContext->PortController != WDF_NO_HANDLE)
    {
        // Direct UcmTcpciCx to stop the port controller and then delete the backing object.
        UcmTcpciPortControllerStop(deviceContext->PortController);
        WdfObjectDelete(deviceContext->PortController);
        deviceContext->PortController = WDF_NO_HANDLE;
    }

    // Close the I2C controller.
    I2CClose(deviceContext);

    TRACE_FUNC_EXIT(TRACE_DEVICE);
    return status;
}
