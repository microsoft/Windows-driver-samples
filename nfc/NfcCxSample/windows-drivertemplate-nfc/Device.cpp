#include <new>

#include "Device.h"

// [Required]
// Called when the device is provisioned during system boot or when a new device is plugged-in while the
// system is running.
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdfdriver/nc-wdfdriver-evt_wdf_driver_device_add
NTSTATUS DeviceContext::AddDevice(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    (void)Driver;

    NTSTATUS status;

    // Configure the NFC Class Extension.
    // Note: The NFC_CX_TRANSPORT_TYPE is only used for telemetry purposes. It does not affect the behavior of
    // the driver.
    NFC_CX_CLIENT_CONFIG nfcCxConfig;
    NFC_CX_CLIENT_CONFIG_INIT(&nfcCxConfig, NFC_CX_TRANSPORT_CUSTOM);

#ifdef ENABLE_IF_IMPLEMENTING_CUSTOM_IOCTLS
    nfcCxConfig.EvtNfcCxDeviceIoControl = IoControl;
#endif

    // Provide the NCI Write callback, which is called by the NFC CX when it has an NCI packet that needs to
    // be sent to the NFC Controller.
    nfcCxConfig.EvtNfcCxWriteNciPacket = WriteNciPacket;

    // Instruct the NFC CX to read the existing NCI configuration values and only update them if necessary.
    nfcCxConfig.DriverFlags = NFC_CX_DRIVER_ENABLE_EEPROM_WRITE_PROTECTION;

    status = NfcCxDeviceInitConfig(DeviceInit, &nfcCxConfig);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Create the PnP power callbacks configuration.
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = PrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = ReleaseHardware;
    pnpCallbacks.EvtDeviceD0Entry = D0Entry;
    pnpCallbacks.EvtDeviceD0Exit = D0Exit;

    // Set the PnP power callbacks.
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

    // Create WDF object attributes for the device.
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DeviceContext);
    deviceAttributes.EvtDestroyCallback = Destroy;

    // Create the device.
    WDFDEVICE device;
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Get a pointer to the raw memory that WDF allocated for the Device class.
    DeviceContext* context = DeviceGetContext(device);

    // Initialize the raw memory using C++'s placement new operator.
    // https://en.cppreference.com/w/cpp/language/new
    new (context) DeviceContext(device);

    // Let the NFC Class Extension initialize the device.
    status = NfcCxDeviceInitialize(device);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Create the RF config. (Enable everything.)
    NFC_CX_RF_DISCOVERY_CONFIG discoveryConfig;
    NFC_CX_RF_DISCOVERY_CONFIG_INIT(&discoveryConfig);
    discoveryConfig.PollConfig = NFC_CX_POLL_NFC_A | NFC_CX_POLL_NFC_B | NFC_CX_POLL_NFC_F_212 | NFC_CX_POLL_NFC_F_424 | NFC_CX_POLL_NFC_15693 | NFC_CX_POLL_NFC_ACTIVE | NFC_CX_POLL_NFC_A_KOVIO;
    discoveryConfig.NfcIPMode = NFC_CX_NFCIP_NFC_A | NFC_CX_NFCIP_NFC_F_212 | NFC_CX_NFCIP_NFC_F_424 | NFC_CX_NFCIP_NFC_ACTIVE | NFC_CX_NFCIP_NFC_ACTIVE_A | NFC_CX_NFCIP_NFC_ACTIVE_F_212 | NFC_CX_NFCIP_NFC_ACTIVE_F_424;
    discoveryConfig.NfcIPTgtMode = NFC_CX_NFCIP_TGT_NFC_A | NFC_CX_NFCIP_TGT_NFC_F | NFC_CX_NFCIP_TGT_NFC_ACTIVE_A | NFC_CX_NFCIP_TGT_NFC_ACTIVE_F;
    discoveryConfig.NfcCEMode = NFC_CX_CE_NFC_A | NFC_CX_CE_NFC_B | NFC_CX_CE_NFC_F;

    // Set the RF config.
    status = NfcCxSetRfDiscoveryConfig(device, &discoveryConfig);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

#ifdef ENABLE_IF_USING_SEQUENCE_HANDLER_CALLBACKS
    // Set the NFC Class Extension sequence handlers.
    for (int sequenceType = 0; sequenceType != SequenceMaximum; ++sequenceType)
    {
        status = NfcCxRegisterSequenceHandler(device, NFC_CX_SEQUENCE(sequenceType), SequenceHandler);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }
#endif

    return STATUS_SUCCESS;
}

DeviceContext::DeviceContext(_In_ WDFDEVICE Device) :
    _Device(Device)
{
}

// [Required]
// Called when the memory for the class is about to be freed by WDF.
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdfobject/nc-wdfobject-evt_wdf_object_context_destroy
void DeviceContext::Destroy(
    _In_ WDFOBJECT Object)
{
    // Manually call the destructor for the class.
    // This mirrors the use of the placement new operator in `DeviceContext::AddDevice`.
    DeviceContext* context = DeviceGetContext(Object);
    context->~DeviceContext();
}

// [Likely required]
// Called when the device's hardware resources are ready to be initialized.
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdfdevice/nc-wdfdevice-evt_wdf_device_prepare_hardware
NTSTATUS DeviceContext::PrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    (void)Device;
    (void)ResourcesRaw;
    (void)ResourcesTranslated;

    // FIX ME:
    // Initialize the hardware so that it is ready to accept NCI packets.

    return STATUS_SUCCESS;
}

// [Likely required]
// Called when the device's hardware resources are no longer accessible.
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdfdevice/nc-wdfdevice-evt_wdf_device_release_hardware
NTSTATUS DeviceContext::ReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    (void)Device;
    (void)ResourcesTranslated;

    // FIX ME:
    // If neccessary, free any resources created by PrepareHardware.

    return STATUS_SUCCESS;
}

// [Likely required]
// Called when the NFC Controller is entering the fully powered-on state.
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdfdevice/nc-wdfdevice-evt_wdf_device_d0_entry
NTSTATUS DeviceContext::D0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    (void)PreviousState;

    NTSTATUS status;

    // Invoke the HostActionStart event, so that the NFC Class Extension initializes the NFC Controller (by
    // sending the relevant NCI packets).
    NFC_CX_HARDWARE_EVENT eventArgs = {};
    eventArgs.HostAction = HostActionStart;

    status = NfcCxHardwareEvent(Device, &eventArgs);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

// [Likely required]
// Called when the NFC Controller is entering a low power state.
//
// By default, the NFC Class Extension will prevent this callback from being invoked if there is an client
// process that may be using the NFC Controller. So it is okay for the NFC Controller to be fully
// uninitialized here.
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdfdevice/nc-wdfdevice-evt_wdf_device_d0_exit
NTSTATUS DeviceContext::D0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    (void)TargetState;

    NTSTATUS status;

    // Invoke the HostActionStop event, so that the NFC Class Extension uninitializes the NFC Controller (by sending
    // the relevant NCI packets).
    NFC_CX_HARDWARE_EVENT eventArgs = {};
    eventArgs.HostAction = HostActionStop;

    status = NfcCxHardwareEvent(Device, &eventArgs);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

// [Optional]
// Called when certain state transitions occur within the NFC Class Extension state machine. This can be used to send
// custom commands to the NFC Controller if neccessary.
//
// https://docs.microsoft.com/windows-hardware/drivers/nfc/sequence-handling
void DeviceContext::SequenceHandler(
    _In_ WDFDEVICE Device,
    _In_ NFC_CX_SEQUENCE Sequence,
    _In_ PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ WDFCONTEXT CompletionContext)
{
    (void)Sequence;

    // Nothing to do. So complete the sequence handler immediately.
    // Note: CompletionRoutine may be called asynchronously.
    CompletionRoutine(Device, STATUS_SUCCESS, 0, CompletionContext);
}

// [Optional]
// Can be used to implement custom IOCTLs.
//
// The NFC Class Extension registers for the default I/O queue and so it gets the first crack at handling all I/O requests.
// If this callback is enabled, any IOCTL the NFC Class Extension doesn't recognize will be forwarded here.
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/nfccx/nc-nfccx-evt_nfc_cx_device_io_control
void DeviceContext::IoControl(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG  IoControlCode)
{
    (void)Device;
    (void)OutputBufferLength;
    (void)InputBufferLength;
    (void)IoControlCode;

    // No custom IOCTLs are currently supported. So complete all I/O requests with a standard error.
    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
}

// [Required]
// Called by the NFC Class Extension when an NCI packet must be sent to the NFC Controller.
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/nfccx/nc-nfccx-evt_nfc_cx_write_nci_packet
void DeviceContext::WriteNciPacket(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request)
{
    (void)Device;

    NTSTATUS status;

    // Get the NCI packet.
    void* nciPacket;
    size_t nciPacketLength;
    status = WdfRequestRetrieveInputBuffer(Request, 0, &nciPacket, &nciPacketLength);
    if (!NT_SUCCESS(status))
    {
        WdfRequestComplete(Request, status);
        return;
    }

    // FIX ME:
    // Send NCI packet to NFC Controller hardware using the relevant bus API.
    // For example,
    //   - I2C or SPI: https://docs.microsoft.com/windows-hardware/drivers/spb/spb-peripheral-device-drivers
    //   - USB: https://docs.microsoft.com/windows-hardware/drivers/usbcon/usb-driver-development-guide

    // FIX ME:
    // Complete I/O request with STATUS_SUCCESS if NCI packet is succesfully sent to the NFC Controller.
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

// FIX ME:
// Ensure NfcCxNciReadNotification is called when the NFC Controller needs to send an NCI packet to the driver.
// This is usually done in response to a hardware notification. For example,
//   - GPIO interrupt (I2C or SPI): https://docs.microsoft.com/windows-hardware/drivers/gpio/gpio-interrupts
//   - USB continuous reader: https://docs.microsoft.com/windows-hardware/drivers/usbcon/how-to-use-the-continous-reader-for-getting-data-from-a-usb-endpoint--umdf-/
//
// https://docs.microsoft.com/windows-hardware/drivers/ddi/content/nfccx/nf-nfccx-nfccxncireadnotification

// A helper function that forwards NCI packets from the NFC Controller to the NFC Class Extension driver.
//
// NOTE: If the NCI packet is already packaged within an existing WDFMEMORY, then the NfcCxNciReadNotification
// function can be called directly.
NTSTATUS DeviceContext::ReadNciPacket(
    _In_reads_bytes_(nciPacketLength) void* nciPacket,
    _In_ size_t nciPacketLength)
{
    NTSTATUS status;

    if (!_NciReadMemory)
    {
        // Create the WDFMEMORY object that will be used for all NCI reads.
        WDF_OBJECT_ATTRIBUTES memoryAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
        memoryAttributes.ParentObject = _Device;

        status = WdfMemoryCreatePreallocated(&memoryAttributes, nciPacket, nciPacketLength, &_NciReadMemory);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }
    else
    {
        // Re-assign the WDFMEMORY to point to the new NCI packet.
        status = WdfMemoryAssignBuffer(_NciReadMemory, nciPacket, nciPacketLength);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    // Note: NfcCxNciReadNotification does not store a reference to the WDFMEMORY object passed to it. So it
    // is safe to free the NCI packet's memory after the call has completed.
    status = NfcCxNciReadNotification(_Device, _NciReadMemory);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}
