#pragma once

#include <windows.h>
#include <wdf.h>
#include <NfcCx.h>

class DeviceContext
{
public:
    // Implementation of `EvtDriverDeviceAdd`.
    static NTSTATUS AddDevice(
        _In_ WDFDRIVER Driver,
        _Inout_ PWDFDEVICE_INIT DeviceInit);

private:
    DeviceContext(_In_ WDFDEVICE Device);
    ~DeviceContext() = default;

    // Implementation of `EvtDestroyCallback`.
    static void Destroy(
        _In_ WDFOBJECT Object);

    // Implementation of `EvtDevicePrepareHardware`.
    static NTSTATUS PrepareHardware(
        _In_ WDFDEVICE Device,
        _In_ WDFCMRESLIST ResourcesRaw,
        _In_ WDFCMRESLIST ResourcesTranslated);

    // Implementation of `EvtDeviceReleaseHardware`.
    static NTSTATUS ReleaseHardware(
        _In_ WDFDEVICE Device,
        _In_ WDFCMRESLIST ResourcesTranslated);

    // Implementation of `EvtDeviceD0Entry`.
    static NTSTATUS D0Entry(
        _In_ WDFDEVICE Device,
        _In_ WDF_POWER_DEVICE_STATE PreviousState);

    // Implementation of `EvtDeviceD0Exit`.
    static NTSTATUS D0Exit(
        _In_ WDFDEVICE Device,
        _In_ WDF_POWER_DEVICE_STATE TargetState);

    // Implementation of `EvtNfcCxSequenceHandler`.
    static void SequenceHandler(
        _In_ WDFDEVICE Device,
        _In_ NFC_CX_SEQUENCE Sequence,
        _In_ PFN_NFC_CX_SEQUENCE_COMPLETION_ROUTINE CompletionRoutine,
        _In_opt_ WDFCONTEXT CompletionContext);

    static void IoControl(
        _In_ WDFDEVICE Device,
        _In_ WDFREQUEST Request,
        _In_ size_t OutputBufferLength,
        _In_ size_t InputBufferLength,
        _In_ ULONG IoControlCode);

    // Implementation of `EvtNfcCxWriteNciPacket`.
    static void WriteNciPacket(
        _In_ WDFDEVICE Device,
        _In_ WDFREQUEST Request);

    NTSTATUS ReadNciPacket(
        _In_reads_bytes_(nciPacketLength) void* nciPacket,
        _In_ size_t nciPacketLength);

    const WDFDEVICE _Device = nullptr;
    WDFMEMORY _NciReadMemory = nullptr;
};

// Define the DeviceGetContext() function which can be used to get a pointer the DeviceContext from a WDFDEVICE object.
// Equivalent to:
//   DeviceContext* DeviceGetContext(WDFOBJECT Handle) { ... }
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DeviceContext, DeviceGetContext);
