/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CircuitDevice.cpp

Abstract:

    Raw PDO for ACX circuits. This file contains routines to create Device
    and handle pnp requests

Environment:

    Kernel mode

--*/

#include "private.h"
#include <devguid.h>
#include "stdunk.h"
#include "modulecircuit.h"

#include "CircuitDevice.h"

#include "SdcaVXuTestInterface.h"

#ifndef __INTELLISENSE__
#include "CircuitDevice.tmh"
#endif

PAGED_CODE_SEG
NTSTATUS
SDCAVXu_CreateCircuitDevice(
    _In_ WDFDEVICE Device
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE circuitDevice = NULL;

    auto exit = scope_exit([&status, &circuitDevice]() {
        if (!NT_SUCCESS(status))
        {
            if (circuitDevice != NULL)
            {
                WdfObjectDelete(circuitDevice);
            }
        }
    });

    //
    // Create a child audio device for this circuit.
    //
    PWDFDEVICE_INIT devInit = NULL;
    devInit = WdfPdoInitAllocate(Device);
    RETURN_NTSTATUS_IF_TRUE(NULL == devInit, STATUS_INSUFFICIENT_RESOURCES);

    auto devInit_free = scope_exit([&devInit, &status]() {
        WdfDeviceInitFree(devInit);
    });

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddHardwareID(devInit, &CircuitHardwareId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignDeviceID(devInit, &CircuitDeviceId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddCompatibleID(devInit, &CircuitCompatibleId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignInstanceID(devInit, &CircuitInstanceId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignContainerID(devInit, &CircuitContainerId));


    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddDeviceText(devInit,
        &CircuitDeviceDescription,
        &CircuitDeviceLocation,
        0x409));

    WdfPdoInitSetDefaultLocale(devInit, 0x409);

    //
    // Allow ACX to add any pre-requirement it needs on this device.
    //
    ACX_DEVICEINIT_CONFIG acxDevInitCfg;
    ACX_DEVICEINIT_CONFIG_INIT(&acxDevInitCfg);
    acxDevInitCfg.Flags |= AcxDeviceInitConfigRawDevice;
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitInitialize(devInit, &acxDevInitCfg));

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = SdcaXuCircuit_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = SdcaXuCircuit_EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = SdcaXuCircuit_EvtDeviceSelfManagedIoInit;
    WdfDeviceInitSetPnpPowerEventCallbacks(devInit, &pnpPowerCallbacks);

    //
    // Specify a context for this circuit device.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    attributes.ParentObject = Device;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_CIRCUIT_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuCircuit_EvtDeviceContextCleanup;
    attributes.ExecutionLevel = WdfExecutionLevelPassive;
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&devInit, &attributes, &circuitDevice));

    //
    // devInit attached to device, no need to free
    //
    devInit_free.release();

    //
    // Tell the framework to set the NoDisplayInUI in the DeviceCaps so
    // that the device does not show up in Device Manager.
    //
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.NoDisplayInUI = WdfTrue;
    WdfDeviceSetPnpCapabilities(circuitDevice, &pnpCaps);

    //
    // Init circuit's device context.
    //
    PSDCAXU_CIRCUIT_DEVICE_CONTEXT circuitDeviceContext;
    circuitDeviceContext = GetCircuitDeviceContext(circuitDevice);
    ASSERT(circuitDeviceContext != NULL);
    circuitDeviceContext->FirstTimePrepareHardware = TRUE;

    //
    // Allow ACX to add any post-requirement it needs on this device.
    //
    ACX_DEVICE_CONFIG devCfg;
    ACX_DEVICE_CONFIG_INIT(&devCfg);
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitialize(circuitDevice, &devCfg));

    //
    // Add circuitDevice to Device's dynamic circuit device list.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddCircuitDevice(Device, circuitDevice));

    PSDCAXU_DEVICE_CONTEXT deviceContext;
    deviceContext = GetSdcaXuDeviceContext(Device);
    deviceContext->CircuitDevice = circuitDevice;

    //
    // Add the RAWCONTROL interface to this device
    //
    RETURN_NTSTATUS_IF_FAILED(SdcaXu_ConfigureTestInterface(circuitDevice, &GUID_DEVINTERFACE_SDCAVXU_TEST_RAWCONTROL));

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtIoctlInterfaceTest(
    _In_ WDFDEVICE  Device,
    _In_ WDFREQUEST Request
)
{
    PAGED_CODE();

    size_t cbOutputBuffer = 0;
    ULONG * input = nullptr;
    ULONG * output = nullptr;
    ULONG temp = 0;

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO, L"Received test IOCTL for device %p", Device);

    RETURN_NTSTATUS_IF_FAILED(WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), (PVOID*)&input, nullptr));

    RETURN_NTSTATUS_IF_FAILED(WdfRequestRetrieveOutputBuffer(Request, 0, (PVOID*)&output, &cbOutputBuffer));
    if (cbOutputBuffer == 0)
    {
        WdfRequestSetInformation(Request, sizeof(ULONG));
        return STATUS_BUFFER_OVERFLOW;
    }
    else if (cbOutputBuffer < sizeof(ULONG))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    temp = ~(*input);

    DrvLogInfo(g_SDCAVXuLog, FLAG_INFO, L"Input received: %x; output being sent back: %x", *input, temp);

    *output = temp;

    WdfRequestSetInformation(Request, sizeof(ULONG));
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
VOID
SdcaXu_EvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     /*OutputBufferLength*/,
    _In_ size_t     /*InputBufferLength*/,
    _In_ ULONG      IoControlCode
)
{
    // The IO Queue for this driver is created with
    //     attributes.ExecutionLevel = WdfExecutionLevelPassive;
    _Analysis_assume_(KeGetCurrentIrql() == PASSIVE_LEVEL);
    PAGED_CODE();

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    switch (IoControlCode)
    {
    case IOCTL_SDCAVXU_INTERFACE_TEST:
        status = SdcaXu_EvtIoctlInterfaceTest(device, Request);
        break;
    }

    WdfRequestComplete(Request, status);
}

//
// SdcaXu_ConfigureTestInterface
//
// This function will:
//   1. Add a queue to the given device that will handle the test IOCTL
//   2. Create a device interface so the device can be located through SetupApi methods
// An alternative to using WdfDeviceCreateDeviceInterface would be to use WdfDeviceCreateSymbolicLink
// with a well-formed name
//
PAGED_CODE_SEG
NTSTATUS
SdcaXu_ConfigureTestInterface(
    _In_    WDFDEVICE Device,
    _In_    PCGUID Interface
)
{
    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ExecutionLevel = WdfExecutionLevelPassive;

    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = SdcaXu_EvtIoDeviceControl;

    RETURN_NTSTATUS_IF_FAILED(WdfIoQueueCreate(Device, &queueConfig, &attributes, nullptr));

    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreateDeviceInterface(Device,
                                                             Interface,
                                                             NULL));
    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuCircuit_EvtDevicePrepareHardware(
    _In_ WDFDEVICE      Device,
    _In_ WDFCMRESLIST   ResourceList,
    _In_ WDFCMRESLIST   ResourceListTranslated
)
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    PSDCAXU_CIRCUIT_DEVICE_CONTEXT devCtx;
    devCtx = GetCircuitDeviceContext(Device);
    ASSERT(devCtx != NULL);

    if (!devCtx->FirstTimePrepareHardware)
    {
        //
        // This is a rebalance. Validate the circuit resources and
        // if needed, delete and re-create the circuit.
        // The sample driver doesn't use resources, thus the existing
        // circuits are kept.
        //
        return STATUS_SUCCESS;
    }


    //
    // Set child's power policy.
    //
    RETURN_NTSTATUS_IF_FAILED(SdcaXuCircuit_SetPowerPolicy(Device));


    RETURN_NTSTATUS_IF_FAILED(SdcaXu_CreateModuleCircuit(Device));

    //
    // Keep track this is not the first time this callback was called.
    //
    devCtx->FirstTimePrepareHardware = FALSE;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuCircuit_SetPowerPolicy(
    _In_ WDFDEVICE      Device
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    //WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;

    PAGED_CODE();

    //
    // Init the idle policy structure.
    //
    //WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCanWakeFromS0);
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeout = IDLE_POWER_TIMEOUT;
    idleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;

    status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuCircuit_EvtDeviceReleaseHardware(
    _In_ WDFDEVICE      Device,
    _In_ WDFCMRESLIST   ResourceListTranslated
)
/*++

Routine Description:

    In this callback, the driver releases the h/w resources allocated in the
    prepare h/w callback.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    PSDCAXU_CIRCUIT_DEVICE_CONTEXT devCtx;
    devCtx = GetCircuitDeviceContext(Device);
    ASSERT(devCtx != NULL);
    UNREFERENCED_PARAMETER(devCtx);


    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuCircuit_EvtDeviceSelfManagedIoInit(
    _In_ WDFDEVICE      Device
)
/*++

Routine Description:

     In this callback, the driver does one-time init of self-managed I/O data.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    PAGED_CODE();

    PSDCAXU_CIRCUIT_DEVICE_CONTEXT devCtx;
    devCtx = GetCircuitDeviceContext(Device);
    ASSERT(devCtx != NULL);
    UNREFERENCED_PARAMETER(devCtx);

    return STATUS_SUCCESS;
}

#pragma code_seg()
VOID
SdcaXuCircuit_EvtDeviceContextCleanup(
    _In_ WDFOBJECT      WdfDevice
)
/*++

Routine Description:

    In this callback, it cleans up device context.

Arguments:

    WdfDevice - WDF device object

Return Value:

    NULL

--*/
{
    WDFDEVICE device;
    PSDCAXU_CIRCUIT_DEVICE_CONTEXT devCtx;

    device = (WDFDEVICE)WdfDevice;
    devCtx = GetCircuitDeviceContext(device);
    ASSERT(devCtx != NULL);
}


