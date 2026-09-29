/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Capture.cpp

Abstract:

    Plug and Play module. This file contains routines to handle pnp requests.

Environment:

    Kernel mode

--*/

#include "private.h"
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>

#include "capture.h"

#include "streamengine.h"

#include "AudioFormats.h"

#ifndef __INTELLISENSE__
#include "capture.tmh"
#endif

PAGED_CODE_SEG
NTSTATUS
SdcaXuC_EvtAcxPinSetDataFormat (
    _In_    ACXPIN          Pin,
    _In_    ACXDATAFORMAT   DataFormat
    )
{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(Pin);
    UNREFERENCED_PARAMETER(DataFormat);


    return STATUS_NOT_SUPPORTED;
}

#pragma code_seg()
VOID
SdcaXuC_EvtPinContextCleanup(
    _In_ WDFOBJECT      WdfPin
   )
/*++

Routine Description:

    In this callback, it cleans up pin context.

Arguments:

    WdfDevice - WDF device object

Return Value:

    NULL

--*/
{
    UNREFERENCED_PARAMETER(WdfPin);
}

VOID SdcaXuC_EvtCircuitContextCleanup(_In_ WDFOBJECT Object)
{
    ACXCIRCUIT circuit = (ACXCIRCUIT)Object;
    PSDCAXU_CAPTURE_CIRCUIT_CONTEXT cirCtx = GetCaptureCircuitContext(circuit);

    if (cirCtx->CircuitConfig)
    {
        ExFreePoolWithTag(cirCtx->CircuitConfig, DRIVER_TAG);
        cirCtx->CircuitConfig = NULL;
    }

    return;
}

PAGED_CODE_SEG
VOID
SdcaXuC_EvtCircuitRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
    )
/*++

Routine Description:

    This function is an example of a preprocess routine.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverContext);
    
    ASSERT(Object != NULL);
    ASSERT(DriverContext);
    ASSERT(Request);


    //
    // Just give the request back to ACX.
    //
    (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
}

PAGED_CODE_SEG
_Use_decl_annotations_
VOID
SdcaXuC_EvtStreamRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
)
/*++

Routine Description:

    This function is an example of a preprocess routine.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverContext);
    
    ASSERT(Object != NULL);
    ASSERT(DriverContext);
    ASSERT(Request);


    //
    // Just give the request back to ACX.
    //
    (VOID)AcxStreamDispatchAcxRequest((ACXSTREAM)Object, Request);
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuC_SetPowerPolicy(
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
SdcaXuC_EvtDevicePrepareHardware(
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

    PSDCAXU_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(Device);
    ASSERT(devCtx != NULL);

    if (!devCtx->FirstTimePrepareHardware)
    {
        //
        // This is a rebalance. Validate the circuit resources and 
        // if needed, delete and re-create the circuit.
        // The sample driver doens't use resources, thus the existing 
        // circuits are kept.
        //
        return STATUS_SUCCESS;
    }

    //
    // Set child's power policy.
    //
    RETURN_NTSTATUS_IF_FAILED(SdcaXuC_SetPowerPolicy(Device));

    //
    // Add circuit to child's list.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddCircuit(Device, devCtx->Circuit));

    //
    // Keep track this is not the first time this callback was called.
    //
    devCtx->FirstTimePrepareHardware = FALSE;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuC_EvtDeviceReleaseHardware(
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

    PSDCAXU_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(Device);
    ASSERT(devCtx != NULL);
    UNREFERENCED_PARAMETER(devCtx);


    return status;
}

PAGED_CODE_SEG
VOID
SdcaXuC_EvtDeviceContextCleanup(
    _In_ WDFOBJECT      WdfDevice
)
/*++

Routine Description:

    In this callback, it cleans up capture device context.

Arguments:

    WdfDevice - WDF device object

Return Value:

    NULL

--*/
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(WdfDevice);
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuC_EvtDeviceSelfManagedIoInit(
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

    PSDCAXU_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(Device);
    ASSERT(devCtx != NULL);
    UNREFERENCED_PARAMETER(devCtx);

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateCaptureDevice(
    _In_ WDFDEVICE Device,
    _Out_ WDFDEVICE* CaptureDevice
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE captureDevice = NULL;

    PAGED_CODE();

    auto exit = scope_exit([&status, &captureDevice]() {
        if (!NT_SUCCESS(status))
        {
            if (captureDevice != NULL)
            {
                WdfObjectDelete(captureDevice);
            }
        }
    });

    *CaptureDevice = NULL;

    //
    // Create a child audio device for this circuit.
    //
    PWDFDEVICE_INIT devInit = NULL;
    devInit = WdfPdoInitAllocate(Device);
    RETURN_NTSTATUS_IF_TRUE(NULL == devInit, STATUS_MEMORY_NOT_ALLOCATED);

    auto devInit_free = scope_exit([&devInit, &status]() {
        WdfDeviceInitFree(devInit);
    });

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddHardwareID(devInit, &CaptureHardwareId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignDeviceID(devInit, &CaptureDeviceId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddCompatibleID(devInit, &CaptureCompatibleId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignInstanceID(devInit, &CaptureInstanceId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignContainerID(devInit, &CaptureContainerId));


    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddDeviceText(devInit,
        &CaptureDeviceDescription,
        &CaptureDeviceLocation,
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
    pnpPowerCallbacks.EvtDevicePrepareHardware = SdcaXuC_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = SdcaXuC_EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = SdcaXuC_EvtDeviceSelfManagedIoInit;
    WdfDeviceInitSetPnpPowerEventCallbacks(devInit, &pnpPowerCallbacks);

    //
    // Specify a context for this capture device.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_CAPTURE_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuC_EvtDeviceContextCleanup;
    attributes.ExecutionLevel = WdfExecutionLevelPassive;
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&devInit, &attributes, &captureDevice));

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
    WdfDeviceSetPnpCapabilities(captureDevice, &pnpCaps);

    //
    // Init capture's device context.
    //
    PSDCAXU_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(captureDevice);
    ASSERT(devCtx != NULL);
    UNREFERENCED_PARAMETER(devCtx);

    //
    // Allow ACX to add any post-requirement it needs on this device.
    //
    ACX_DEVICE_CONFIG devCfg;
    ACX_DEVICE_CONFIG_INIT(&devCfg);
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitialize(captureDevice, &devCfg));

    //
    // Set output value.
    //
    *CaptureDevice = captureDevice;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_AddDynamicCapture(
    _In_ WDFDEVICE                      Device,
    _In_ PSDCAXU_ACX_CIRCUIT_CONFIG     CircuitConfig
)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Create a device to associated with this circuit.
    //
    WDFDEVICE captureDevice = NULL;
    RETURN_NTSTATUS_IF_FAILED(SdcaXu_CreateCaptureDevice(Device, &captureDevice));
    auto deviceFree = scope_exit([&captureDevice]() {
        WdfObjectDelete(captureDevice);
    });

    ASSERT(captureDevice);
    PSDCAXU_CAPTURE_DEVICE_CONTEXT captureDevCtx;
    captureDevCtx = GetCaptureDeviceContext(captureDevice);
    ASSERT(captureDevCtx);

    //
    // Create a capture circuit associated with this child device.
    //
    ACXCIRCUIT captureCircuit = NULL;
    RETURN_NTSTATUS_IF_FAILED(SdcaXu_CreateCaptureCircuit(captureDevice, CircuitConfig, &captureCircuit));

    captureDevCtx->Circuit = captureCircuit;
    captureDevCtx->FirstTimePrepareHardware = TRUE;

    //
    // Add circuit to device's dynamic circuit device list.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddCircuitDevice(Device, captureDevice));

    // Successfully created circuit for dynamic deivce
    // Do not delete
    deviceFree.release();

    PSDCAXU_DEVICE_CONTEXT devCtx = GetSdcaXuDeviceContext(Device);
    for (ULONG i = 0; i < ARRAYSIZE(devCtx->EndpointDevices); ++i)
    {
        if (devCtx->EndpointDevices[i].CircuitDevice == nullptr)
        {
            DrvLogInfo(g_SDCAVXuLog, FLAG_DDI, L"XU Device %p adding capture circuit device %p with component ID %!GUID! Uri %ls",
                Device, captureDevice, &CircuitConfig->ComponentID,
                CircuitConfig->ComponentUri.Buffer ? CircuitConfig->ComponentUri.Buffer : L"<none>");
            devCtx->EndpointDevices[i].CircuitDevice = captureDevice;
            devCtx->EndpointDevices[i].CircuitId = CircuitConfig->ComponentID;
            if (CircuitConfig->ComponentUri.Length > 0)
            {
                USHORT cbAlloc = CircuitConfig->ComponentUri.Length + sizeof(WCHAR);
                // protect against overflow
                if (CircuitConfig->ComponentUri.Length % 2 != 0 ||
                    cbAlloc < CircuitConfig->ComponentUri.Length)
                {
                    RETURN_NTSTATUS_IF_FAILED(STATUS_INVALID_PARAMETER);
                }

                PWCHAR circuitUri = (PWCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, cbAlloc, DRIVER_TAG);
                if (!circuitUri)
                {
                    RETURN_NTSTATUS_IF_FAILED(STATUS_INSUFFICIENT_RESOURCES);
                }

                devCtx->EndpointDevices[i].CircuitUri.Buffer = circuitUri;
                devCtx->EndpointDevices[i].CircuitUri.MaximumLength = cbAlloc;
                devCtx->EndpointDevices[i].CircuitUri.Length = 0;
                RtlCopyUnicodeString(&devCtx->EndpointDevices[i].CircuitUri, &CircuitConfig->ComponentUri);
            }
            break;
        }
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateCaptureCircuit(
    _In_    WDFDEVICE                   Device,
    _In_    PSDCAXU_ACX_CIRCUIT_CONFIG  CircuitConfig,
    _Out_   ACXCIRCUIT                  *Circuit
)
/*++

Routine Description:

    This routine builds the SdcaXu capture circuit.

Return Value:

    NT status value

--*/
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Get a CircuitInit structure.
    //
    PACXCIRCUIT_INIT circuitInit = NULL;
    circuitInit = AcxCircuitInitAllocate(Device);
    RETURN_NTSTATUS_IF_TRUE(NULL == circuitInit, STATUS_MEMORY_NOT_ALLOCATED);
    auto circuitInit_free = scope_exit([&circuitInit]() {
        AcxCircuitInitFree(circuitInit);
    });

    //
    // Init output value.
    //
    *Circuit = NULL;

    //
    // Copy Circuit configuration
    //
    PSDCAXU_ACX_CIRCUIT_CONFIG pCircuitConfig = NULL;
    pCircuitConfig = (PSDCAXU_ACX_CIRCUIT_CONFIG)ExAllocatePool2(POOL_FLAG_NON_PAGED,
        CircuitConfig->cbSize,
        DRIVER_TAG);
    RETURN_NTSTATUS_IF_TRUE(NULL == pCircuitConfig, STATUS_MEMORY_NOT_ALLOCATED);
    auto circuitConfig_free = scope_exit([&pCircuitConfig]() {
        ExFreePoolWithTag(pCircuitConfig, DRIVER_TAG);
        });

    RtlCopyMemory(pCircuitConfig, CircuitConfig, CircuitConfig->cbSize);

    // Remap UNICODE_STRING.Buffer
    // buffer for unicode string begins immediately after SdcaXuAcxCircuitConfig
    RETURN_NTSTATUS_IF_TRUE_MSG(pCircuitConfig->cbSize < (sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) + pCircuitConfig->CircuitName.MaximumLength),
        STATUS_INVALID_PARAMETER, L"CircuitConfig->cbSize = %d Required = %d",
        pCircuitConfig->cbSize,
        (int)(sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) + pCircuitConfig->CircuitName.MaximumLength));

    pCircuitConfig->CircuitName.Buffer = (PWCH)(pCircuitConfig + 1);

    //
    // Create a circuit.
    //

    //
    // Add circuit identifiers.
    //
    if (!IsEqualGUID(pCircuitConfig->ComponentID, GUID_NULL))
    {
        AcxCircuitInitSetComponentId(circuitInit, &pCircuitConfig->ComponentID);
    }

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignComponentUri(circuitInit, &pCircuitConfig->ComponentUri));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignName(circuitInit, &pCircuitConfig->CircuitName));

    //
    // Add circuit type.
    //
    AcxCircuitInitSetCircuitType(circuitInit, AcxCircuitTypeCapture);

    //
    // Assign the circuit's pnp-power callbacks.
    //
    {
        ACX_CIRCUIT_PNPPOWER_CALLBACKS powerCallbacks;
        ACX_CIRCUIT_PNPPOWER_CALLBACKS_INIT(&powerCallbacks);
        powerCallbacks.EvtAcxCircuitPowerUp = SdcaXuC_EvtCircuitPowerUp;
        powerCallbacks.EvtAcxCircuitPowerDown = SdcaXuC_EvtCircuitPowerDown;
        AcxCircuitInitSetAcxCircuitPnpPowerCallbacks(circuitInit, &powerCallbacks);
    }

    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
        circuitInit,
        SdcaXuC_EvtCircuitRequestPreprocess,
        (ACXCONTEXT)AcxRequestTypeAny, // dbg only
        AcxRequestTypeAny,
        NULL,
        AcxItemIdNone));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxCreateStreamCallback(
        circuitInit,
        SdcaXuC_EvtCircuitCreateStream));

    /*
    //
    // Add properties, events and methods.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignProperties(circuitInit,
                                            CircuitProperties,
                                            CircuitPropertiesCount));
    */

    //
    // Disable default Stream Bridge handling in ACX
    // Create stream handler will add Stream Bridge
    // to support Object-bag forwarding
    //
    AcxCircuitInitDisableDefaultStreamBridgeHandling(circuitInit);

    //
    // Create the circuit.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    ACXCIRCUIT circuit;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_CAPTURE_CIRCUIT_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuC_EvtCircuitContextCleanup;
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitCreate(Device, &attributes, &circuitInit, &circuit));

    // circuitInit is now associated with circuit and will be managed with
    // circuit lifetime.
    circuitInit_free.release();

    SDCAXU_CAPTURE_CIRCUIT_CONTEXT* circuitCtx;
    ASSERT(circuit != NULL);
    circuitCtx = GetCaptureCircuitContext(circuit);
    ASSERT(circuitCtx);

    circuitCtx->CircuitConfig = pCircuitConfig;
    circuitConfig_free.release();

    //
    // Post circuit creation initialization.
    //

    //
    // Add two custom circuit elements. Note that driver doesn't need to 
    // perform this step if it doesn't want to expose any circuit elements.
    //

    //
    // Create 1st custom circuit-element.
    //
    ACX_ELEMENT_CONFIG elementCfg;
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;

    const int numElements = 2;
    ACXELEMENT elements[numElements] = { 0 };
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(circuit, &attributes, &elementCfg, &elements[0]));

    ASSERT(elements[0] != NULL);
    SDCAXU_ELEMENT_CONTEXT* elementCtx;
    elementCtx = GetSdcaXuElementContext(elements[0]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Create 2nd custom circuit-element.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(circuit, &attributes, &elementCfg, &elements[1]));

    ASSERT(elements[1] != NULL);
    elementCtx = GetSdcaXuElementContext(elements[1]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Add the circuit elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(circuit, elements, SIZEOF_ARRAY(elements)));

    //
    // Create capture pin. AcxCircuit creates the other pin by default.
    //
    ACX_PIN_CALLBACKS pinCallbacks;
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat = SdcaXuC_EvtAcxPinSetDataFormat;

    ACX_PIN_CONFIG pinCfg;
    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = AcxPinTypeSource;
    pinCfg.Communication = AcxPinCommunicationNone;
    pinCfg.Category = &KSCATEGORY_AUDIO;
    pinCfg.PinCallbacks = &pinCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PIN_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuC_EvtPinContextCleanup;
    attributes.ParentObject = circuit;

    ACXPIN pin;
    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(circuit, &attributes, &pinCfg, &pin));

    ASSERT(pin != NULL);
    SDCAXU_PIN_CONTEXT* pinCtx;
    pinCtx = GetSdcaXuPinContext(pin);
    ASSERT(pinCtx);

    // When the downstream pin connects to the Class driver, we'll
    // copy formats from the Class driver (instead of hardcoding
    // formats here)

    //
    // Add capture pin, using default pin id (0) 
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    ///////////////////////////////////////////////////////////
    //
    // Create bridge pin. AcxCircuit creates the other pin by default.
    //
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinConnected = SdcaXu_EvtPinConnected;

    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = AcxPinTypeSink;
    pinCfg.Communication = AcxPinCommunicationNone;
    pinCfg.Category = &KSCATEGORY_AUDIO;
    pinCfg.PinCallbacks = &pinCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PIN_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuC_EvtPinContextCleanup;
    attributes.ParentObject = circuit;

    pin = NULL;
    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(circuit, &attributes, &pinCfg, &pin));

    ASSERT(pin != NULL);
    pinCtx = GetSdcaXuPinContext(pin);
    ASSERT(pinCtx);

    //
    // Add brige pin, using default pin id (1)
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    //
    // Add a stream bridge to the bridge pin to propagate the stream obj-bags.
    //
    {
        PCGUID  inModes[] =
        {
            &NULL_GUID, // Match every mode.
        };

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = pin;

        ACX_STREAM_BRIDGE_CONFIG streamBridgeConfig;
        ACX_STREAM_BRIDGE_CONFIG_INIT(&streamBridgeConfig);

        streamBridgeConfig.Flags |= AcxStreamBridgeForwardInStreamVarArguments;
        streamBridgeConfig.InModesCount = ARRAYSIZE(inModes);
        streamBridgeConfig.InModes = inModes;
        streamBridgeConfig.OutMode = &NULL_GUID; // Use the MODE associated the in-stream.

        ACXSTREAMBRIDGE streamBridge = NULL;
        RETURN_NTSTATUS_IF_FAILED(AcxStreamBridgeCreate(circuit, &attributes, &streamBridgeConfig, &streamBridge));

        RETURN_NTSTATUS_IF_FAILED(AcxPinAddStreamBridges(pin, &streamBridge, 1));
    }

    //
    // Explicitly connect the circuit/elements. Note that driver doens't 
    // need to perform this step when circuit/elements are connected in the 
    // same order as they were added to the circuit. By default ACX connects
    // the elements starting from the sink circuit pin and ending with the 
    // source circuit pin for both capture and capture devices.
    //
    // circuit.pin[default_sink]    -> 1st element.pin[default_in]
    // 1st element.pin[default_out] -> 2nd element.pin[default_in]
    // 2nd element.pin[default_out] -> circuit.pin[default_source]
    //
    const int numConnections = numElements + 1;
    ACX_CONNECTION connections[numConnections];
    ACX_CONNECTION_INIT(&connections[0], circuit, elements[0]);
    ACX_CONNECTION_INIT(&connections[1], elements[0], elements[1]);
    ACX_CONNECTION_INIT(&connections[2], elements[1], circuit);

    //
    // Add the connections linking circuit to elements.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddConnections(circuit, connections, SIZEOF_ARRAY(connections)));

    //
    // Set output value.
    //
    *Circuit = circuit;

    //
    // Done. 
    //

    return status;
}

#pragma code_seg()
_Use_decl_annotations_
NTSTATUS
SdcaXuC_EvtCircuitPowerUp (
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    // Do not page out.
    
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(PreviousState);
    
    return STATUS_SUCCESS;
}
    
PAGED_CODE_SEG
_Use_decl_annotations_
NTSTATUS
SdcaXuC_EvtCircuitPowerDown (
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(TargetState);
    
    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuC_EvtCircuitCreateStream(
    _In_    WDFDEVICE       Device,
    _In_    ACXCIRCUIT      Circuit,
    _In_    ACXPIN          Pin,
    _In_    PACXSTREAM_INIT StreamInit,
    _In_    ACXDATAFORMAT   StreamFormat,
    _In_    const GUID    * SignalProcessingMode,
    _In_    ACXOBJECTBAG    VarArguments
    )
/*++

Routine Description:

    This routine create a stream for the specified circuit.

Return Value:

    NT status value

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Pin);
    UNREFERENCED_PARAMETER(SignalProcessingMode);
    UNREFERENCED_PARAMETER(VarArguments);

    ASSERT(IsEqualGUID(*SignalProcessingMode, AUDIO_SIGNALPROCESSINGMODE_RAW));
    
    PSDCAXU_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(Device);
    ASSERT(devCtx != NULL);

    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxRequestPreprocessCallback(
                                            StreamInit, 
                                            SdcaXuC_EvtStreamRequestPreprocess,
                                            (ACXCONTEXT)AcxRequestTypeAny, // dbg only
                                            AcxRequestTypeAny,
                                            NULL, 
                                            AcxItemIdNone));

    /*
    //
    // Add properties, events and methods.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignProperties(StreamInit,
                                         StreamProperties,
                                         StreamPropertiesCount));
    */
    
    //
    // Init streaming callbacks.
    //
    ACX_STREAM_CALLBACKS streamCallbacks;
    ACX_STREAM_CALLBACKS_INIT(&streamCallbacks);
    streamCallbacks.EvtAcxStreamPrepareHardware     = SdcaXu_EvtStreamPrepareHardware;
    streamCallbacks.EvtAcxStreamReleaseHardware     = SdcaXu_EvtStreamReleaseHardware;
    streamCallbacks.EvtAcxStreamRun                 = SdcaXu_EvtStreamRun;
    streamCallbacks.EvtAcxStreamPause               = SdcaXu_EvtStreamPause;
    streamCallbacks.EvtAcxStreamAssignDrmContentId  = SdcaXu_EvtStreamAssignDrmContentId;
        
    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxStreamCallbacks(StreamInit, &streamCallbacks));

    //
    // Create the stream.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_STREAM_CONTEXT);
    attributes.EvtDestroyCallback = SdcaXu_EvtStreamDestroy;
    ACXSTREAM stream;
    RETURN_NTSTATUS_IF_FAILED(AcxStreamCreate(Device, Circuit, &attributes, &StreamInit, &stream));

    CCaptureStreamEngine *streamEngine = NULL;
    streamEngine = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CCaptureStreamEngine(stream, StreamFormat);
    RETURN_NTSTATUS_IF_TRUE(NULL == streamEngine, STATUS_MEMORY_NOT_ALLOCATED);
    auto stream_scope = scope_exit([&streamEngine]() {
        delete streamEngine;
    });

    SDCAXU_STREAM_CONTEXT *streamCtx;
    streamCtx = GetSdcaXuStreamContext(stream);
    ASSERT(streamCtx);
    streamCtx->StreamEngine = (PVOID)streamEngine;
    stream_scope.release();

    //
    // Post stream creation initialization.
    //

    ACXELEMENT elements[2] = {0};
    ACX_ELEMENT_CONFIG elementCfg;
    //
    // Create 1st custom stream-elements.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_ELEMENT_CONTEXT);
    attributes.ParentObject = stream;
    
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(stream, &attributes, &elementCfg, &elements[0]));

    ASSERT(elements[0] != NULL);
    SDCAXU_ELEMENT_CONTEXT *elementCtx;
    elementCtx = GetSdcaXuElementContext(elements[0]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);
    
    //
    // Create 2nd custom stream-elements.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_ELEMENT_CONTEXT);
    attributes.ParentObject = stream;
    
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(stream, &attributes, &elementCfg, &elements[1]));

    ASSERT(elements[1] != NULL);
    elementCtx = GetSdcaXuElementContext(elements[1]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Add stream elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(stream, elements, SIZEOF_ARRAY(elements)));

    //
    // Done. 
    //
    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_AddCaptures(
    _In_ WDFDEVICE                      Device,
    _In_ PSDCAXU_ACX_CIRCUIT_CONFIG     CircuitConfig
)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Add dynamic capture circuit using raw PDO
    //
    status = SdcaXu_AddDynamicCapture(Device, CircuitConfig);

    return status;
}


