/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Capture.cpp

Abstract:

    Contains ACX Capture factory and circuit

Environment:

    Kernel mode

--*/

#include "private.h"
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>
#include "streamengine.h"
#include "soundwirecontroller.h"
#include "sdcastreaming.h"
#include "CircuitHelper.h"

#include "AudioFormats.h"

#ifndef __INTELLISENSE__
#include "capture.tmh"
#endif

ACX_PROPERTY_ITEM KwsProperties[] =
{
    {
        &KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_DEVICE_CAPABILITY,
        ACX_PROPERTY_ITEM_FLAG_GET,
        &CodecC_EvtCircuitDeviceKwsCapability, // Event to call
        NULL, // Reserved
        0, // ControlCb
        sizeof(DEVICE_KWS_CAPABILITY_DESCRIPTOR) // ValueCb
    },
    {
        &KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_VAD_CAPABILITY,
        ACX_PROPERTY_ITEM_FLAG_GET,
        &CodecC_EvtCircuitVadCapability, // Event to call
        NULL, // Reserved
        0, // ControlCb
        sizeof(VAD_DESCRIPTOR) // ValueCb
    },
    {
        &KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_VAD_ENTITIES,
        ACX_PROPERTY_ITEM_FLAG_GET,
        &CodecC_EvtCircuitVadEntities, // Event to call
        NULL, // Reserved
        0, // ControlCb
        sizeof(VAD_ENTITIES) // ValueCb
    },
    {
        &KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_ACCESS_EVENTS,
        ACX_PROPERTY_ITEM_FLAG_SET,
        &CodecC_EvtCircuitSetKwsAccessEvents, // Event to call
        NULL, // Reserved
        0, // ControlCb
        sizeof(SDCA_KWS_NOTIFICATIONS) // ValueCb
    },
    {
        &KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_CONFIGURE_VAD_PORT,
        ACX_PROPERTY_ITEM_FLAG_SET,
        &CodecC_EvtCircuitConfigureVadPort, // Event to call
        NULL, // Reserved
        0, // ControlCb
        sizeof(SDCA_KWS_PREPARE_PARAMS) // ValueCb
    },
    {
        &KSPROPERTYSETID_SdcaKws,
        KSPROPERTY_SDCAKWS_CLEANUP_VAD_PORT,
        ACX_PROPERTY_ITEM_FLAG_SET,
        &CodecC_EvtCircuitCleanupVadPort, // Event to call
        // No parameters - can only have one
    },
};

_Use_decl_annotations_
PAGED_CODE_SEG
VOID
CodecC_EvtCircuitVadCapability(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    NTSTATUS                            status = STATUS_NOT_SUPPORTED;
    ACX_REQUEST_PARAMETERS              params;
    ULONG_PTR                           outDataCb = 0;
    PVAD_DESCRIPTOR                     value;
    ULONG                               valueCb;
    ULONG_PTR                           minSize;
    PCODEC_CAPTURE_CIRCUIT_CONTEXT      circuitCtx;
    ULONG                               formatCount = 0;

    PAGED_CODE();

    circuitCtx = GetCaptureCircuitContext((ACXCIRCUIT)Object);

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    ASSERT(params.Type == AcxRequestTypeProperty);
    ASSERT(params.Parameters.Property.Verb == AcxPropertyVerbGet);

    value = (PVAD_DESCRIPTOR)params.Parameters.Property.Value;
    valueCb = params.Parameters.Property.ValueCb;

    //
    // Compute min size.
    //
    minSize = sizeof(VAD_DESCRIPTOR);

    //
    // Sample only supports 1 format
    //
    formatCount = 1;

    // Note the VAD_DESCRIPTOR already has room for 1, hence subtracting that here
    minSize += (formatCount - ANYSIZE_ARRAY) * sizeof(WAVEFORMATEXTENSIBLE);

    if (valueCb == 0)
    {
        outDataCb = minSize;
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (valueCb < minSize)
    {
        outDataCb = 0;
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        // 
        // Reset buffer.
        //
        RtlZeroMemory(value, valueCb);

        // It's safe for us to use the KwsDataFormat directly in AcxDataFormatGetWaveFormatExtensible
        // because we control it and know it will have a proper WAVEFORMATEXTENSIBLE value.
        RtlCopyMemory(value->Format, AcxDataFormatGetWaveFormatExtensible(circuitCtx->KwsDataFormat), sizeof(WAVEFORMATEXTENSIBLE));

        value->FormatCount = formatCount;

        //
        // All done.
        //
        outDataCb = minSize;
        status = STATUS_SUCCESS;
    }

    WdfRequestCompleteWithInformation(Request, status, outDataCb);
}


_Use_decl_annotations_
PAGED_CODE_SEG
VOID
CodecC_EvtCircuitVadEntities(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    NTSTATUS                            status = STATUS_NOT_SUPPORTED;
    ACX_REQUEST_PARAMETERS              params;
    ULONG_PTR                           outDataCb = 0;
    PVAD_DESCRIPTOR                     value;
    ULONG                               valueCb;
    ULONG_PTR                           minSize;
    PCODEC_CAPTURE_CIRCUIT_CONTEXT      circuitCtx;

    PAGED_CODE();

    circuitCtx = GetCaptureCircuitContext((ACXCIRCUIT)Object);

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    ASSERT(params.Type == AcxRequestTypeProperty);
    ASSERT(params.Parameters.Property.Verb == AcxPropertyVerbGet);

    value = (PVAD_DESCRIPTOR)params.Parameters.Property.Value;
    valueCb = params.Parameters.Property.ValueCb;

    // we're going to return 0 entities, so only the base structure
    // is needed.
    minSize = sizeof(VAD_ENTITIES);

    if (valueCb == 0)
    {
        outDataCb = minSize;
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (valueCb < minSize)
    {
        outDataCb = 0;
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        // 
        // Reset buffer.
        //
        RtlZeroMemory(value, valueCb);

        // we do not have disco info for this sample driver, so
        // we have no entities to copy, but an empty list is sufficient
        // for testing.

        outDataCb = minSize;
        status = STATUS_SUCCESS;
    }

    WdfRequestCompleteWithInformation(Request, status, outDataCb);
}

_Use_decl_annotations_
PAGED_CODE_SEG
VOID
CodecC_EvtCircuitDeviceKwsCapability(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    NTSTATUS                            status = STATUS_NOT_SUPPORTED;
    ACX_REQUEST_PARAMETERS              params;
    ULONG_PTR                           outDataCb = 0;
    PDEVICE_KWS_CAPABILITY_DESCRIPTOR   value;
    ULONG                               valueCb;
    ULONG_PTR                           minSize;
    PCODEC_CAPTURE_CIRCUIT_CONTEXT      circuitCtx;

    PAGED_CODE();

    circuitCtx = GetCaptureCircuitContext((ACXCIRCUIT)Object);

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    ASSERT(params.Type == AcxRequestTypeProperty);
    ASSERT(params.Parameters.Property.Verb == AcxPropertyVerbGet);

    value = (PDEVICE_KWS_CAPABILITY_DESCRIPTOR)params.Parameters.Property.Value;
    valueCb = params.Parameters.Property.ValueCb;

    //
    // Compute min size.
    //

    minSize = sizeof(DEVICE_KWS_CAPABILITY_DESCRIPTOR);

    if (valueCb == 0)
    {
        outDataCb = minSize;
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (valueCb < minSize)
    {
        outDataCb = 0;
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        // 
        // Reset buffer.
        //
        RtlZeroMemory(value, valueCb);

        // Get the Device KWS Capabilities
        // In this sample, just return that Buffered is supported
        value->DataPathsSupported = SupportedDataPathsBufferedRaw;

        //
        // All done.
        //
        outDataCb = minSize;
        status = STATUS_SUCCESS;
    }

    WdfRequestCompleteWithInformation(Request, status, outDataCb);
}

_Use_decl_annotations_
PAGED_CODE_SEG
VOID
CodecC_EvtCircuitSetKwsAccessEvents(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    NTSTATUS                        status = STATUS_NOT_SUPPORTED;
    ACX_REQUEST_PARAMETERS          params;
    ULONG_PTR                       outDataCb = 0; // default no size info
    PSDCA_KWS_NOTIFICATIONS         value;
    ULONG                           valueCb;
    ULONG                           minSize;
    PCODEC_CAPTURE_CIRCUIT_CONTEXT  circuitCtx;

    PAGED_CODE();

    circuitCtx = GetCaptureCircuitContext((ACXCIRCUIT)Object);

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    ASSERT(params.Type == AcxRequestTypeProperty);
    ASSERT(params.Parameters.Property.Verb == AcxPropertyVerbSet);

    minSize = sizeof(SDCA_KWS_NOTIFICATIONS);

    value = (PSDCA_KWS_NOTIFICATIONS)params.Parameters.Property.Value;
    valueCb = params.Parameters.Property.ValueCb;

    if (valueCb == 0)
    {
        outDataCb = minSize;
        status = STATUS_BUFFER_OVERFLOW;
        goto exit;
    }

    if (valueCb < minSize)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    circuitCtx->KwsSuspendEvent = value->Suspend;
    circuitCtx->KwsResumeEvent = value->Resume;

    status = STATUS_SUCCESS;

exit:

    WdfRequestCompleteWithInformation(Request, status, outDataCb);
}

_Use_decl_annotations_
PAGED_CODE_SEG
VOID
CodecC_EvtCircuitConfigureVadPort(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    NTSTATUS                        status = STATUS_NOT_SUPPORTED;
    ACX_REQUEST_PARAMETERS          params;
    ULONG_PTR                       outDataCb = 0; // default no size info
    PSDCA_KWS_PREPARE_PARAMS        value;
    ULONG                           valueCb;
    ULONG                           minSize;
    PCODEC_CAPTURE_CIRCUIT_CONTEXT  circuitCtx;

    PAGED_CODE();

    circuitCtx = GetCaptureCircuitContext((ACXCIRCUIT)Object);

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    ASSERT(params.Type == AcxRequestTypeProperty);
    ASSERT(params.Parameters.Property.Verb == AcxPropertyVerbSet);

    if (circuitCtx->KwsActiveVadStream)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    minSize = sizeof(SDCA_KWS_PREPARE_PARAMS);

    value = (PSDCA_KWS_PREPARE_PARAMS)params.Parameters.Property.Value;
    valueCb = params.Parameters.Property.ValueCb;

    if (valueCb == 0)
    {
        outDataCb = minSize;
        status = STATUS_BUFFER_OVERFLOW;
        goto exit;
    }

    if (valueCb < minSize)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    if (value->DetectionFormat.Format.cbSize > sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
    {
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    // Set up the hardware for KWS
    circuitCtx->KwsActiveVadStream = TRUE;
    status = STATUS_SUCCESS;

exit:

    WdfRequestCompleteWithInformation(Request, status, outDataCb);
}

_Use_decl_annotations_
PAGED_CODE_SEG
VOID
CodecC_EvtCircuitCleanupVadPort(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    NTSTATUS                            status = STATUS_NOT_SUPPORTED;
    ACX_REQUEST_PARAMETERS              params;
    ULONG_PTR                           outDataCb = 0; // default no size info
    PCODEC_CAPTURE_CIRCUIT_CONTEXT      circuitCtx;

    PAGED_CODE();

    circuitCtx = GetCaptureCircuitContext((ACXCIRCUIT)Object);

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    ASSERT(params.Type == AcxRequestTypeProperty);
    ASSERT(params.Parameters.Property.Verb == AcxPropertyVerbSet);

    if (!circuitCtx->KwsActiveVadStream)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    // Deconfigure hardware
    circuitCtx->KwsActiveVadStream = FALSE;
    status = STATUS_SUCCESS;

exit:

    WdfRequestCompleteWithInformation(Request, status, outDataCb);
}

PAGED_CODE_SEG
NTSTATUS
CodecC_EvtAcxPinSetDataFormat(
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
CodecC_EvtPinContextCleanup(
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

PAGED_CODE_SEG
VOID
CodecC_EvtCircuitRequestPreprocess(
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
VOID
CodecC_EvtStreamRequestPreprocess(
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
CodecC_AddCaptures(
    _In_ WDFDRIVER      Driver,
    _In_ WDFDEVICE      Device
)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    //
    // Add a static capture device.
    //
    RETURN_NTSTATUS_IF_FAILED(CodecC_AddStaticCapture(Device));

    return status;
}

PAGED_CODE_SEG
NTSTATUS
CodecC_AddStaticCapture(
    _In_ WDFDEVICE      Device
)
{
    NTSTATUS status = STATUS_SUCCESS;
        
    PAGED_CODE();

    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext(Device);
    ASSERT(devCtx != NULL);

    //
    // Alloc audio context to current device.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_CAPTURE_DEVICE_CONTEXT);
    PCODEC_CAPTURE_DEVICE_CONTEXT captureDevCtx;
    RETURN_NTSTATUS_IF_FAILED(WdfObjectAllocateContext(Device, &attributes, (PVOID*)&captureDevCtx));

    ASSERT(captureDevCtx);

    //
    // Create a capture circuit associated with this device.
    //
    ACXCIRCUIT captureCircuit = NULL;
    RETURN_NTSTATUS_IF_FAILED(CodecC_CreateCaptureCircuit(Device, &captureCircuit));

    RETURN_NTSTATUS_IF_FAILED(Codec_SdcaXuSetCaptureEndpointConfig(Device, captureCircuit));

    devCtx->Capture = captureCircuit;

    return status;
}

EXTERN_C const GUID DECLSPEC_SELECTANY CODEC_CIRCUIT_CAPTURE_GUID;
EXTERN_C const GUID DECLSPEC_SELECTANY EXTENSION_CIRCUIT_CAPTURE_GUID;
EXTERN_C const GUID DECLSPEC_SELECTANY SYSTEM_CONTAINER_GUID;

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetCaptureEndpointConfig(
    _In_ WDFDEVICE Device,
    _In_ ACXCIRCUIT Circuit
)
{
    PAGED_CODE();

    DrvLogEnter(g_SDCAVCodecLog);

    NTSTATUS status = STATUS_SUCCESS;

    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext(Device);
    ASSERT(devCtx != NULL);

    DECLARE_CONST_UNICODE_STRING(circuitName, L"ExtensionMicrophone0");
    DECLARE_CONST_UNICODE_STRING(circuitUri, EXT_CAPTURE_CIRCUIT_URI);

#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "memory is freed by scope_exit")
    PSDCAXU_ACX_CIRCUIT_CONFIG exCircuitConfig = (PSDCAXU_ACX_CIRCUIT_CONFIG)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) + circuitName.MaximumLength,
        DRIVER_TAG);
    RETURN_NTSTATUS_IF_TRUE(NULL == exCircuitConfig, STATUS_INSUFFICIENT_RESOURCES);
    auto exConfigFree = scope_exit([&exCircuitConfig]() {
        ExFreePoolWithTag(exCircuitConfig, DRIVER_TAG);
        });

    //
    // Provide circuit configuration to SDCA XU driver
    // SDCA XU driver will generate circuits to match this configuration
    //
    if (devCtx->SdcaXuData.bSdcaXu)
    {
        exCircuitConfig->cbSize = sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) + circuitName.MaximumLength;

        exCircuitConfig->CircuitName = circuitName;
        exCircuitConfig->CircuitName.Buffer = (PWCH)(exCircuitConfig + 1);
        RtlCopyMemory(exCircuitConfig->CircuitName.Buffer, circuitName.Buffer, circuitName.MaximumLength);

        exCircuitConfig->CircuitContext = Circuit;
        exCircuitConfig->CircuitType = AcxCircuitTypeCapture;
        exCircuitConfig->ContainerID = SYSTEM_CONTAINER_GUID;
        exCircuitConfig->ComponentID = EXTENSION_CIRCUIT_CAPTURE_GUID;
        exCircuitConfig->ComponentUri = circuitUri;

        PSDCAXU_INTERFACE_V0101 exInterface = &devCtx->SdcaXuData.ExtensionInterface;
        PVOID exContext = devCtx->SdcaXuData.ExtensionInterface.InterfaceHeader.Context;

        RETURN_NTSTATUS_IF_FAILED(exInterface->EvtSetEndpointConfig(exContext, SdcaXuEndpointConfigTypeAcxCircuitConfig, exCircuitConfig, exCircuitConfig->cbSize));
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
CodecC_CreateCaptureCircuit(
    _In_     WDFDEVICE      Device,
    _Out_    ACXCIRCUIT *   Circuit
)
/*++

Routine Description:

    This routine builds the CODEC capture circuit.

Return Value:

    NT status value

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
    
    //
    // Init output value.
    //
    *Circuit = NULL;

    ///////////////////////////////////////////////////////////
    //
    // Create a circuit.
    //

    //
    // Get a CircuitInit structure.
    //
    PACXCIRCUIT_INIT circuitInit = NULL;
    circuitInit = AcxCircuitInitAllocate(Device);
    RETURN_NTSTATUS_IF_TRUE(NULL == circuitInit, STATUS_NO_MEMORY);
    auto circuitInitScope = scope_exit([&circuitInit]() {
        AcxCircuitInitFree(circuitInit);
    });

    //
    // Add circuit identifiers.
    //
    AcxCircuitInitSetComponentId(circuitInit, &CODEC_CIRCUIT_CAPTURE_GUID);

    DECLARE_CONST_UNICODE_STRING(circuitUri, CAPTURE_CIRCUIT_URI);
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignComponentUri(circuitInit, &circuitUri));

    DECLARE_CONST_UNICODE_STRING(circuitName, L"Microphone0");
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignName(circuitInit, &circuitName));

    //
    // Add circuit type.
    //
    AcxCircuitInitSetCircuitType(circuitInit, AcxCircuitTypeCapture);

    //
    // Assign the circuit's pnp-power callbacks.
    //
    ACX_CIRCUIT_PNPPOWER_CALLBACKS powerCallbacks;
    ACX_CIRCUIT_PNPPOWER_CALLBACKS_INIT(&powerCallbacks);
    powerCallbacks.EvtAcxCircuitPowerUp = CodecC_EvtCircuitPowerUp;
    powerCallbacks.EvtAcxCircuitPowerDown = CodecC_EvtCircuitPowerDown;
    AcxCircuitInitSetAcxCircuitPnpPowerCallbacks(circuitInit, &powerCallbacks);

    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
                                            circuitInit, 
                                            CodecC_EvtCircuitRequestPreprocess,
                                            (ACXCONTEXT)AcxRequestTypeAny, // dbg only
                                            AcxRequestTypeAny,
                                            NULL, 
                                            AcxItemIdNone));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxCreateStreamCallback(
                                            circuitInit, 
                                            CodecC_EvtCircuitCreateStream));
    
    //
    // Add properties, events and methods.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignProperties(circuitInit,
                                                             KwsProperties,
                                                             ARRAYSIZE(KwsProperties)));

    //
    // Create the circuit.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_CAPTURE_CIRCUIT_CONTEXT);   
    ACXCIRCUIT circuit;
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitCreate(Device, &attributes, &circuitInit, &circuit));
    circuitInitScope.release();

    ASSERT(circuit != NULL);
    CODEC_CAPTURE_CIRCUIT_CONTEXT *circuitCtx;
    circuitCtx = GetCaptureCircuitContext(circuit);
    ASSERT(circuitCtx);
        
    //
    // Post circuit creation initialization.
    //

    ///////////////////////////////////////////////////////////
    //
    // Add two custom circuit elements. Note that driver doesn't need to 
    // perform this step if it doesn't want to expose any circuit elements.
    //

    //
    // Create 1st custom circuit-element.
    //
    ACX_ELEMENT_CONFIG elementCfg;
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;
    
    const int numElements = 2;
    ACXELEMENT elements[numElements] = {0};
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(circuit, &attributes, &elementCfg, &elements[0]));

    ASSERT(elements[0] != NULL);
    CODEC_ELEMENT_CONTEXT *elementCtx;
    elementCtx = GetCodecElementContext(elements[0]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);
    
    //
    // Create 2nd custom circuit-element.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;
    
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(circuit, &attributes, &elementCfg, &elements[1]));

    ASSERT(elements[1] != NULL);
    elementCtx = GetCodecElementContext(elements[1]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Add the circuit elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(circuit, elements, SIZEOF_ARRAY(elements)));

    ///////////////////////////////////////////////////////////
    // Create Capture Pin, using default pin id.
    // Acx Circuit will create other pin by default.
    //
    // Allocate the formats this circuit supports. Use formats without
    // channel mask for capture.
    //
    // PCM:44100 channel:2 24in32
    ACX_DATAFORMAT_CONFIG formatCfg;
    ACX_DATAFORMAT_CONFIG_INIT_KS(&formatCfg, &Pcm44100c2_24in32_nomask);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_FORMAT_CONTEXT);
    attributes.ParentObject = circuit;

    ACXDATAFORMAT formatPcm44100c2_24in32nomask;
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatCreate(Device, &attributes, &formatCfg, &formatPcm44100c2_24in32nomask));

    CODEC_FORMAT_CONTEXT *formatCtx;
    formatCtx = GetCodecFormatContext(formatPcm44100c2_24in32nomask);
    ASSERT(formatCtx);
    UNREFERENCED_PARAMETER(formatCtx);

    // PCM:48000 channel:2 24in32
    ACX_DATAFORMAT_CONFIG_INIT_KS(&formatCfg, &Pcm48000c2_24in32_nomask);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_FORMAT_CONTEXT);
    attributes.ParentObject = circuit;

    ACXDATAFORMAT formatPcm48000c2_24in32nomask;
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatCreate(Device, &attributes, &formatCfg, &formatPcm48000c2_24in32nomask));

    formatCtx = GetCodecFormatContext(formatPcm48000c2_24in32nomask);
    ASSERT(formatCtx);
    UNREFERENCED_PARAMETER(formatCtx);

    // This is the format we'll report support for with KWS. Note that DSP uses 4ch; that includes
    // 2ch from the hardware + 2ch reference
    ACX_DATAFORMAT_CONFIG_INIT_KS(&formatCfg, &Pcm16000c2nomask);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_FORMAT_CONTEXT);
    attributes.ParentObject = circuit;

    ACXDATAFORMAT formatPcm16000c2nomask;
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatCreate(Device, &attributes, &formatCfg, &formatPcm16000c2nomask));

    formatCtx = GetCodecFormatContext(formatPcm16000c2nomask);
    ASSERT(formatCtx);
    UNREFERENCED_PARAMETER(formatCtx);

    ///////////////////////////////////////////////////////////
    //
    // Create Capture Pin. AcxCircuit creates the other pin by default.
    //

    ACX_PIN_CALLBACKS pinCallbacks;
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat = CodecC_EvtAcxPinSetDataFormat;

    ACX_PIN_CONFIG pinCfg;
    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = AcxPinTypeSource;
    pinCfg.Communication = AcxPinCommunicationNone;
    pinCfg.Category = &KSCATEGORY_AUDIO;
    pinCfg.PinCallbacks = &pinCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_PIN_CONTEXT);
    attributes.EvtCleanupCallback = CodecC_EvtPinContextCleanup;
    attributes.ParentObject = circuit;

    ACXPIN pin;
    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(circuit, &attributes, &pinCfg, &pin));

    ASSERT(pin != NULL);
    CODEC_PIN_CONTEXT *pinCtx;
    pinCtx = GetCodecPinContext(pin);
    ASSERT(pinCtx);
    UNREFERENCED_PARAMETER(pinCtx);

    //
    // Add our supported formats to the Default mode for the circuit
    //
    ACXDATAFORMATLIST formatList;
    formatList = AcxPinGetRawDataFormatList(pin);
    RETURN_NTSTATUS_IF_TRUE(NULL == formatList, STATUS_INSUFFICIENT_RESOURCES);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32nomask));

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm48000c2_24in32nomask));

    circuitCtx->KwsDataFormat = formatPcm16000c2nomask;

    // Add Capture Pin, using default pin id. 
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    ///////////////////////////////////////////////////////////
    //
    // Create Bridge Pin.
    //
    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = AcxPinTypeSink;
    pinCfg.Communication = AcxPinCommunicationNone;
    pinCfg.Category = &KSNODETYPE_MICROPHONE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_PIN_CONTEXT);
    attributes.EvtCleanupCallback = CodecR_EvtPinContextCleanup;
    attributes.ParentObject = circuit;
    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(circuit, &attributes, &pinCfg, &pin));

    ASSERT(pin != NULL);

    RETURN_NTSTATUS_IF_FAILED(AddJack(attributes, pin, SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, RGB(0, 0, 0), AcxConnTypeAtapiInternal, AcxGeoLocFront, AcxGenLocPrimaryBox, AcxPortConnIntegratedDevice));

    // Add capture bridge pin
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));



    //
    // Explicitly connect the circuit/elements. Note that driver doens't 
    // need to perform this step when circuit/elements are connected in the 
    // same order as they were added to the circuit. By default ACX connects
    // the elements starting from the sink circuit pin and ending on the 
    // source circuit pin on both render and capture devices.
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
    
    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CodecC_EvtCircuitPowerUp(
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
)
{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(PreviousState);
    
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CodecC_EvtCircuitPowerDown(
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
CodecC_EvtCircuitCreateStream(
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
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Pin);
    UNREFERENCED_PARAMETER(SignalProcessingMode);
    UNREFERENCED_PARAMETER(VarArguments);

    ASSERT(IsEqualGUID(*SignalProcessingMode, AUDIO_SIGNALPROCESSINGMODE_RAW));
    
    PCODEC_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(Device);
    ASSERT(devCtx != NULL);

    DECLARE_CONST_ACXOBJECTBAG_DRIVER_PROPERTY_NAME(msft, TestUI4);
    if (VarArguments)
    {
        // Get the variable arguments parameter and retrive the values set by the DSP object.
        ULONG ui4Value = 0;
        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagRetrieveUI4(VarArguments, &TestUI4, &ui4Value));

        RETURN_NTSTATUS_IF_TRUE(ui4Value == 0, STATUS_UNSUCCESSFUL);

        ui4Value++;

        // Add the modified value back to object bag.
        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUI4(VarArguments, &TestUI4, ui4Value));
    }

    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxRequestPreprocessCallback(
                                            StreamInit, 
                                            CodecC_EvtStreamRequestPreprocess,
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
    streamCallbacks.EvtAcxStreamPrepareHardware     = Codec_EvtStreamPrepareHardware;
    streamCallbacks.EvtAcxStreamReleaseHardware     = Codec_EvtStreamReleaseHardware;
    streamCallbacks.EvtAcxStreamRun                 = Codec_EvtStreamRun;
    streamCallbacks.EvtAcxStreamPause               = Codec_EvtStreamPause;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxStreamCallbacks(StreamInit, &streamCallbacks));
        
    //
    // Create the stream.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_STREAM_CONTEXT);   
    attributes.EvtDestroyCallback = Codec_EvtStreamDestroy;
    ACXSTREAM stream;
    RETURN_NTSTATUS_IF_FAILED(AcxStreamCreate(Device, Circuit, &attributes, &StreamInit, &stream));

    CCaptureStreamEngine *streamEngine = NULL;
    streamEngine = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CCaptureStreamEngine(stream, StreamFormat);
    RETURN_NTSTATUS_IF_TRUE(NULL == streamEngine, STATUS_INSUFFICIENT_RESOURCES);

    CODEC_STREAM_CONTEXT *streamCtx;
    streamCtx = GetCodecStreamContext(stream);
    ASSERT(streamCtx);
    streamCtx->StreamEngine = (PVOID)streamEngine;
    streamEngine = NULL;

    //
    // Post stream creation initialization.
    //

    //
    // Create 1st custom stream-elements.
    //
    ACX_ELEMENT_CONFIG elementCfg;
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_ELEMENT_CONTEXT);
    attributes.ParentObject = stream;
    
    ACXELEMENT elements[2] = {0};
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(stream, &attributes, &elementCfg, &elements[0]));

    ASSERT(elements[0] != NULL);
    CODEC_ELEMENT_CONTEXT *elementCtx;
    elementCtx = GetCodecElementContext(elements[0]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);
    
    //
    // Create 2nd custom stream-elements.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_ELEMENT_CONTEXT);
    attributes.ParentObject = stream;
    
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(stream, &attributes, &elementCfg, &elements[1]));

    ASSERT(elements[1] != NULL);
    elementCtx = GetCodecElementContext(elements[1]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Add stream elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(stream, elements, SIZEOF_ARRAY(elements)));

    return status;
}


