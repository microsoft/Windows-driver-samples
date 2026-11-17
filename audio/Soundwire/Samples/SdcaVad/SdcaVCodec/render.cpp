/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Render.cpp

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
#include "render.tmh"
#endif

//#define CODEC_NEXT_CIRCUIT_STR L"\\??\\ROOT#AcxAmpTestDriver#0000#{2c6bb644-e1ae-47f8-9a2b-1d1fa750f2fa}\\Speaker0"
//#define CODEC_NEXT_CIRCUIT_STR L"\\??\\ROOT#AcxAmpTestDriver#0000#{6994AD04-93EF-11D0-A3CC-00A0C9223196}\\Speaker0"

//#define CODEC_PREVIOUS_CIRCUIT_STR L"\\??\\AcxDspTestDriver#DynamicEnumSpeaker0#1&6244bc4&d&00#{2c6bb644-e1ae-47f8-9a2b-1d1fa750f2fa}\\Speaker0"
//#define CODEC_PREVIOUS_CIRCUIT_STR L"\\??\\AcxDspTestDriver#DynamicEnumSpeaker0#1&6244bc4&0&00#{6994AD04-93EF-11D0-A3CC-00A0C9223196}\\Speaker0"

PAGED_CODE_SEG
VOID
CodecR_EvtPinCInstancesCallback(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
    )
{
    PAGED_CODE();

    // TEMP: for testing only.
    UNREFERENCED_PARAMETER(Object);
    WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);    
}

PAGED_CODE_SEG
VOID
CodecR_EvtPinCTypesCallback(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
    )
{
    PAGED_CODE();

    // TEMP: for testing only.
    UNREFERENCED_PARAMETER(Object);
    WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);    
}

PAGED_CODE_SEG
VOID
CodecR_EvtPinDataFlowCallback(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
    )
{
    PAGED_CODE();

    // TEMP: for testing only.
    UNREFERENCED_PARAMETER(Object);
    WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);    
}

PAGED_CODE_SEG
VOID
CodecR_EvtPinDataRangesCallback(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
    )
{
    PAGED_CODE();

    // TEMP: for testing only.
    UNREFERENCED_PARAMETER(Object);
    WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);    
}

PAGED_CODE_SEG
VOID
CodecR_EvtPinDataIntersectionCallback(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
    )
{
    PAGED_CODE();

    // TEMP: for testing only.
    UNREFERENCED_PARAMETER(Object);
    WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);    
}

PAGED_CODE_SEG
NTSTATUS
CodecR_EvtAcxPinSetDataFormat (
    _In_    ACXPIN          Pin,
    _In_    ACXDATAFORMAT   DataFormat
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Pin);
    UNREFERENCED_PARAMETER(DataFormat);


    return STATUS_NOT_SUPPORTED;
}

PAGED_CODE_SEG
NTSTATUS
CodecR_EvtMuteAssignStateCallback(
    _In_    ACXMUTE Mute,
    _In_    ULONG   Channel,
    _In_    ULONG   State
    )
{
    PAGED_CODE();

    ASSERT(Mute);
    PCODEC_MUTE_ELEMENT_CONTEXT muteCtx = GetCodecMuteElementContext(Mute);
    ASSERT(muteCtx);

    if (Channel != ALL_CHANNELS_ID)
    { 
        muteCtx->MuteState[Channel] = State; 
    }
    else
    {
        for (ULONG i = 0; i < MAX_CHANNELS; ++i)
        {
            muteCtx->MuteState[i] = State;
        }
    }

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
NTAPI
CodecR_EvtMuteRetrieveStateCallback(
    _In_    ACXMUTE Mute,
    _In_    ULONG   Channel,
    _Out_   ULONG   *State
    )
{
    PAGED_CODE();

    ASSERT(Mute);
    PCODEC_MUTE_ELEMENT_CONTEXT muteCtx = GetCodecMuteElementContext(Mute);
    ASSERT(muteCtx);

    if (Channel == ALL_CHANNELS_ID)
    { 
        Channel = 0;
    }

    *State = muteCtx->MuteState[Channel]; 

    return STATUS_SUCCESS;
}

//
// Testing mute element.
//
#pragma code_seg()
VOID
CodecR_EvtMuteTimerFunc(
    _In_ WDFTIMER Timer
    )
{
    PCODEC_MUTE_TIMER_CONTEXT timerCtx = GetCodecMuteTimerContext(Timer);

    ASSERT(timerCtx != NULL);
    ASSERT(timerCtx->MuteElement != NULL);

    PCODEC_MUTE_ELEMENT_CONTEXT muteCtx = GetCodecMuteElementContext(timerCtx->MuteElement);
    ASSERT(muteCtx != NULL);

    // update settings 0 <-> 1 
    for (ULONG i = 0; i < MAX_CHANNELS; ++i)
    {
        muteCtx->MuteState[i] = !muteCtx->MuteState[i];
    }

    AcxMuteChangeStateNotification(timerCtx->MuteElement);
}

PAGED_CODE_SEG
NTSTATUS
CodecR_EvtVolumeAssignLevelCallback(
    _In_    ACXVOLUME   Volume,
    _In_    ULONG       Channel,
    _In_    LONG        VolumeLevel
    )
{
    PAGED_CODE();

    ASSERT(Volume);
    PCODEC_VOLUME_ELEMENT_CONTEXT volumeCtx = GetCodecVolumeElementContext(Volume);
    ASSERT(volumeCtx);

    if (Channel != ALL_CHANNELS_ID)
    {
        volumeCtx->VolumeLevel[Channel] = VolumeLevel;
    }
    else
    {
        for (ULONG i = 0; i < MAX_CHANNELS; ++i)
        {
            volumeCtx->VolumeLevel[i] = VolumeLevel;
        }
    }

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
NTAPI
CodecR_EvtVolumeRetrieveLevelCallback(
    _In_    ACXVOLUME   Volume,
    _In_    ULONG       Channel,
    _Out_   LONG        *VolumeLevel
    )
{
    PAGED_CODE();

    ASSERT(Volume);
    PCODEC_VOLUME_ELEMENT_CONTEXT volumeCtx = GetCodecVolumeElementContext(Volume);
    ASSERT(volumeCtx);

    if (Channel == ALL_CHANNELS_ID)
    {
        Channel = 0;
    }

    *VolumeLevel = volumeCtx->VolumeLevel[Channel];

    return STATUS_SUCCESS;
}

//
// Testing volume element.
//
#pragma code_seg()
VOID
CodecR_EvtVolumeTimerFunc(
    _In_ WDFTIMER Timer
    )
{
    PCODEC_VOLUME_TIMER_CONTEXT timerCtx = GetCodecVolumeTimerContext(Timer);

    ASSERT(timerCtx != NULL);
    ASSERT(timerCtx->VolumeElement != NULL);

    PCODEC_VOLUME_ELEMENT_CONTEXT volumeCtx = GetCodecVolumeElementContext(timerCtx->VolumeElement);
    ASSERT(volumeCtx != NULL);

    // Toggle volume between max and min
    for (ULONG i = 0; i < MAX_CHANNELS; ++i)
    {
        volumeCtx->VolumeLevel[i] = volumeCtx->VolumeLevel[i] == VOLUME_LEVEL_MAXIMUM ? VOLUME_LEVEL_MINIMUM : VOLUME_LEVEL_MAXIMUM;
    }

    AcxVolumeChangeLevelNotification(timerCtx->VolumeElement);
}

#pragma code_seg()
VOID
CodecR_EvtPinContextCleanup(
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
CodecR_EvtCircuitRequestPreprocess(
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
CodecR_EvtStreamRequestPreprocess(
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
CodecR_AddRenders(
    _In_ WDFDRIVER      Driver,
    _In_ WDFDEVICE      Device
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Add a static render device.
    //
    status = CodecR_AddStaticRender(Device);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
CodecR_AddStaticRender(
    _In_ WDFDEVICE      Device
)
{
    PAGED_CODE();

    DrvLogEnter(g_SDCAVCodecLog);

    NTSTATUS status = STATUS_SUCCESS;

    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext(Device);
    ASSERT(devCtx != NULL);

    //
    // Alloc audio context to current device.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_RENDER_DEVICE_CONTEXT);
    PCODEC_RENDER_DEVICE_CONTEXT renderDevCtx;
    RETURN_NTSTATUS_IF_FAILED(WdfObjectAllocateContext(Device, &attributes, (PVOID*)&renderDevCtx));
    ASSERT(renderDevCtx);

    //
    // Create a render circuit associated with this device.
    //
    ACXCIRCUIT renderCircuit = NULL;
    RETURN_NTSTATUS_IF_FAILED(CodecR_CreateRenderCircuit(Device, &renderCircuit));

    RETURN_NTSTATUS_IF_FAILED(Codec_SdcaXuSetRenderEndpointConfig(Device, renderCircuit));

    devCtx->Render = renderCircuit;

    return status;
}

EXTERN_C const GUID DECLSPEC_SELECTANY CODEC_CIRCUIT_RENDER_GUID;
EXTERN_C const GUID DECLSPEC_SELECTANY EXTENSION_CIRCUIT_RENDER_GUID;
EXTERN_C const GUID DECLSPEC_SELECTANY SYSTEM_CONTAINER_GUID;

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetRenderEndpointConfig
(
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

    DECLARE_CONST_UNICODE_STRING(circuitName, L"ExtensionSpeaker0");
    DECLARE_CONST_UNICODE_STRING(circuitUri, EXT_RENDER_CIRCUIT_URI);

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
        exCircuitConfig->CircuitType = AcxCircuitTypeRender;
        exCircuitConfig->ContainerID = SYSTEM_CONTAINER_GUID;
        exCircuitConfig->ComponentID = EXTENSION_CIRCUIT_RENDER_GUID;
        exCircuitConfig->ComponentUri = circuitUri;

        PSDCAXU_INTERFACE_V0101 exInterface = &devCtx->SdcaXuData.ExtensionInterface;
        PVOID exContext = devCtx->SdcaXuData.ExtensionInterface.InterfaceHeader.Context;

        RETURN_NTSTATUS_IF_FAILED(exInterface->EvtSetEndpointConfig(exContext, SdcaXuEndpointConfigTypeAcxCircuitConfig, exCircuitConfig, exCircuitConfig->cbSize));
    }

    return status;
}

// {3CE41646-9BF2-4A9E-B851-D711CAE9AEA8}
DEFINE_GUID(SDCAVADPropsetId,
    0x3ce41646, 0x9bf2, 0x4a9e, 0xb8, 0x51, 0xd7, 0x11, 0xca, 0xe9, 0xae, 0xa8);

typedef enum {
    SDCAVAD_PROPERTY_TEST1,
    SDCAVAD_PROPERTY_TEST2,
    SDCAVAD_PROPERTY_TEST3,
    SDCAVAD_PROPERTY_TEST4,
    SDCAVAD_PROPERTY_TEST5,
    SDCAVAD_PROPERTY_TEST6,
} SDCAVAD_Properties;

PAGED_CODE_SEG
NTSTATUS
CodecR_SDCAVADPropertyTest1(
    _Inout_ PVOID pValue,
    _In_ ULONG ValueCb,
    _Out_ PULONG ValueCbOut
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pValue);
    UNREFERENCED_PARAMETER(ValueCb);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVCodecLog, FLAG_STREAM, L"SDCAVCodec: SDCAVAD_PROPERTY_TEST1");

    *ValueCbOut = 0;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
CodecR_SDCAVADPropertyTest2(
    _Inout_ PVOID pValue,
    _In_ ULONG ValueCb,
    _Out_ PULONG ValueCbOut
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(ValueCb);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVCodecLog, FLAG_STREAM, L"SDCAVCodec: SDCAVAD_PROPERTY_TEST2");

    *((PULONG)pValue) = 10;
    *ValueCbOut = sizeof(ULONG);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
CodecR_SDCAVADPropertyTest5(
    _Inout_ PVOID pValue,
    _In_ ULONG ValueCb,
    _Out_ PULONG ValueCbOut
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pValue);
    UNREFERENCED_PARAMETER(ValueCb);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVCodecLog, FLAG_STREAM, L"SDCAVCodec: SDCAVAD_PROPERTY_TEST5");

    *ValueCbOut = 0;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
CodecR_SDCAVADPropertyTest6(
    _Inout_ PVOID pValue,
    _In_ ULONG ValueCb,
    _Out_ PULONG ValueCbOut
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(ValueCb);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVCodecLog, FLAG_STREAM, L"SDCAVCodec: SDCAVAD_PROPERTY_TEST6");

    *((PULONG)pValue) = 12;
    *ValueCbOut = sizeof(ULONG);

    return status;
}

PAGED_CODE_SEG
VOID
CodecR_EvtPropertyCallback(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Object);

    ACX_REQUEST_PARAMETERS params;
    ACX_REQUEST_PARAMETERS_INIT(&params);

    AcxRequestGetParameters(Request, &params);

    NTSTATUS status = STATUS_SUCCESS;
    PVOID Value = params.Parameters.Property.Value;
    ULONG ValueCb = params.Parameters.Property.ValueCb;
    ULONG ValueCbOut = 0;

    switch (params.Parameters.Property.Id)
    {
    case SDCAVAD_PROPERTY_TEST1:
        status = CodecR_SDCAVADPropertyTest1(Value, ValueCb, &ValueCbOut);
        break;
    case SDCAVAD_PROPERTY_TEST2:
        status = CodecR_SDCAVADPropertyTest2(Value, ValueCb, &ValueCbOut);
        break;
    case SDCAVAD_PROPERTY_TEST5:
        status = CodecR_SDCAVADPropertyTest5(Value, ValueCb, &ValueCbOut);
        break;
    case SDCAVAD_PROPERTY_TEST6:
        status = CodecR_SDCAVADPropertyTest6(Value, ValueCb, &ValueCbOut);
        break;
    default:
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, ValueCbOut);
}

PAGED_CODE_SEG
VOID
CodecR_EvtPropertyVendorSpecificCallback(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Object);

    ACX_REQUEST_PARAMETERS params;
    ACX_REQUEST_PARAMETERS_INIT(&params);

    AcxRequestGetParameters(Request, &params);

    NTSTATUS status = STATUS_SUCCESS;

    // The Class Driver will send IOCTL_SOUNDWIRE_VENDOR_SPECIFIC with Control/Value to the SoundWire Controller.

    PVIRTUAL_STACK_VENDOR_SPECIFIC_CONTROL control = (PVIRTUAL_STACK_VENDOR_SPECIFIC_CONTROL)params.Parameters.Property.Control;
    ULONG controlCb = params.Parameters.Property.ControlCb;

    PVIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA value = (PVIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA)params.Parameters.Property.Value;
    ULONG valueCb = params.Parameters.Property.ValueCb;

    ULONG_PTR information = 0;

    // Validate we have enough control data
    if (controlCb < sizeof(VIRTUAL_STACK_VENDOR_SPECIFIC_CONTROL))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if (control->VendorSpecificSize != sizeof(VIRTUAL_STACK_VENDOR_SPECIFIC_CONTROL))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if (control->VendorSpecificId == VirtualStackVendorSpecificRequestGetTestData)
    {
        if (valueCb == 0 && value == nullptr)
        {
            status = STATUS_BUFFER_OVERFLOW;
            information = sizeof(VIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA);
        }
        else if (valueCb < sizeof(VIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA))
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            value->Test1 = 0x12345678;
            value->Test2 = 0x87654321;
            information = sizeof(VIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA);
        }
    }
    else if (control->VendorSpecificId == VirtualStackVendorSpecificRequestSetTestConfig)
    {
        DrvLogInfo(g_SDCAVCodecLog, FLAG_STREAM, L"SDCAVCodec: VENDOR SPECIFIC Set Test Config %d", control->Config.IsScatterGather);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    WdfRequestCompleteWithInformation(Request, status, information);
}

static ACX_PROPERTY_ITEM g_CircuitProperties[] =
{
    {
        &SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST1,
        ACX_PROPERTY_ITEM_FLAG_SET,
        CodecR_EvtPropertyCallback
    },
    {
        &SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST2,
        ACX_PROPERTY_ITEM_FLAG_GET,
        CodecR_EvtPropertyCallback
    },
    {
        &SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST5,
        ACX_PROPERTY_ITEM_FLAG_SET,
        CodecR_EvtPropertyCallback
    },
    {
        &SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST6,
        ACX_PROPERTY_ITEM_FLAG_GET,
        CodecR_EvtPropertyCallback
    },
    {
        &KSPROPERTYSETID_Sdca,
        KSPROPERTY_SDCA_VENDOR_SPECIFIC,
        ACX_PROPERTY_ITEM_FLAG_GET | ACX_PROPERTY_ITEM_FLAG_SET,
        CodecR_EvtPropertyVendorSpecificCallback
    },
};

PAGED_CODE_SEG
NTSTATUS
CodecR_CreateRenderCircuit(
    _In_     WDFDEVICE      Device,
    _Out_    ACXCIRCUIT *   Circuit
)
/*++

Routine Description:

    This routine builds the CODEC render circuit.

Return Value:

    NT status value

--*/
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVCodecLog);

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
    // Init output value.
    //
    *Circuit = NULL;

    ///////////////////////////////////////////////////////////
    //
    // Create a circuit.
    //

    //
    // Add circuit identifiers.
    //
    AcxCircuitInitSetComponentId(circuitInit, &CODEC_CIRCUIT_RENDER_GUID);

    DECLARE_CONST_UNICODE_STRING(circuitUri, RENDER_CIRCUIT_URI);
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignComponentUri(circuitInit, &circuitUri));

    WDF_OBJECT_ATTRIBUTES attributes;
    ACXCIRCUIT circuit;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_RENDER_CIRCUIT_CONTEXT);
    DECLARE_CONST_UNICODE_STRING(circuitName, L"Speaker0");

    //
    // Add properties, events and methods.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignProperties(circuitInit,
        g_CircuitProperties,
        SIZEOF_ARRAY(g_CircuitProperties)));


    RETURN_NTSTATUS_IF_FAILED(CreateRenderCircuit(circuitInit, circuitName, Device, &circuit));
    circuitInitScope.release();

    CODEC_RENDER_CIRCUIT_CONTEXT *circuitCtx;
    ASSERT(circuit != NULL);
    circuitCtx = GetRenderCircuitContext(circuit);
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
    // Create 1st custom circuit-element (mute element).
    //
    ACX_MUTE_CALLBACKS muteCallbacks;
    ACX_MUTE_CALLBACKS_INIT(&muteCallbacks);
    muteCallbacks.EvtAcxMuteAssignState   = CodecR_EvtMuteAssignStateCallback;
    muteCallbacks.EvtAcxMuteRetrieveState = CodecR_EvtMuteRetrieveStateCallback;

    ACX_MUTE_CONFIG muteCfg;
    ACX_MUTE_CONFIG_INIT(&muteCfg);
    muteCfg.ChannelsCount = MAX_CHANNELS;
    muteCfg.Callbacks = &muteCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_MUTE_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;

    const int numElements = 2;
    ACXELEMENT elements[numElements] = {0};
    RETURN_NTSTATUS_IF_FAILED(AcxMuteCreate(circuit, &attributes, &muteCfg, (ACXMUTE *)&elements[0]));

    ASSERT(elements[0] != NULL);
    CODEC_MUTE_ELEMENT_CONTEXT *muteCtx;
    muteCtx = GetCodecMuteElementContext(elements[0]);
    ASSERT(muteCtx);
    UNREFERENCED_PARAMETER(muteCtx);

    circuitCtx->MuteElement = (ACXMUTE)elements[0];

    //
    // Testing async mute state change.
    //
    {
        WDF_TIMER_CONFIG            timerCfg;
        PCODEC_MUTE_TIMER_CONTEXT   timerCtx;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_MUTE_TIMER_CONTEXT);
        attributes.ParentObject = circuitCtx->MuteElement;

        WDF_TIMER_CONFIG_INIT_PERIODIC(&timerCfg, CodecR_EvtMuteTimerFunc, 4000 /* 4sec in msec */);

        RETURN_NTSTATUS_IF_FAILED(WdfTimerCreate(&timerCfg, &attributes, &muteCtx->Timer));

        ASSERT(muteCtx->Timer);

        timerCtx = GetCodecMuteTimerContext(muteCtx->Timer);
        ASSERT(timerCtx);

        timerCtx->MuteElement = circuitCtx->MuteElement;
    }

    //
    // Create 2nd custom circuit-element (volume element).
    //
    ACX_VOLUME_CALLBACKS volumeCallbacks;
    ACX_VOLUME_CALLBACKS_INIT(&volumeCallbacks);
    volumeCallbacks.EvtAcxVolumeAssignLevel = CodecR_EvtVolumeAssignLevelCallback;
    volumeCallbacks.EvtAcxVolumeRetrieveLevel = CodecR_EvtVolumeRetrieveLevelCallback;

    ACX_VOLUME_CONFIG volumeCfg;
    ACX_VOLUME_CONFIG_INIT(&volumeCfg);
    volumeCfg.ChannelsCount = MAX_CHANNELS;
    volumeCfg.Minimum = VOLUME_LEVEL_MINIMUM;
    volumeCfg.Maximum = VOLUME_LEVEL_MAXIMUM;
    volumeCfg.SteppingDelta = VOLUME_STEPPING;
    volumeCfg.Callbacks = &volumeCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_VOLUME_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxVolumeCreate(circuit, &attributes, &volumeCfg, (ACXVOLUME *)&elements[1]));

    ASSERT(elements[1] != NULL);
    CODEC_VOLUME_ELEMENT_CONTEXT *volumeCtx;
    volumeCtx = GetCodecVolumeElementContext(elements[1]);
    ASSERT(volumeCtx);
    volumeCtx->VolumeLevel[0] = (VOLUME_LEVEL_MAXIMUM + VOLUME_LEVEL_MINIMUM) / 2 / VOLUME_STEPPING * VOLUME_STEPPING;
    volumeCtx->VolumeLevel[1] = (VOLUME_LEVEL_MAXIMUM + VOLUME_LEVEL_MINIMUM) / 2 / VOLUME_STEPPING * VOLUME_STEPPING;

    circuitCtx->VolumeElement = (ACXVOLUME)elements[1];

    //
    // Testing async volume state change.
    //
    {
        WDF_TIMER_CONFIG            timerCfg;
        PCODEC_VOLUME_TIMER_CONTEXT timerCtx;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_VOLUME_TIMER_CONTEXT);
        attributes.ParentObject = circuitCtx->VolumeElement;

        WDF_TIMER_CONFIG_INIT_PERIODIC(&timerCfg, CodecR_EvtVolumeTimerFunc, 4500 /* 4.5sec in msec */);

        RETURN_NTSTATUS_IF_FAILED(WdfTimerCreate(&timerCfg, &attributes, &volumeCtx->Timer));

        ASSERT(volumeCtx->Timer);

        timerCtx = GetCodecVolumeTimerContext(volumeCtx->Timer);
        ASSERT(timerCtx);

        timerCtx->VolumeElement = circuitCtx->VolumeElement;
    }

    //
    // Add the circuit elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(circuit, elements, SIZEOF_ARRAY(elements)));

    ///////////////////////////////////////////////////////////
    //
    // Allocate the formats this circuit supports.
    //
    // PCM:44100 channel:2 24in32
    ACX_DATAFORMAT_CONFIG formatCfg;
    ACX_DATAFORMAT_CONFIG_INIT_KS(&formatCfg, &Pcm44100c2_24in32);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_FORMAT_CONTEXT);
    attributes.ParentObject = circuit;

    ACXDATAFORMAT formatPcm44100c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatCreate(Device, &attributes, &formatCfg, &formatPcm44100c2_24in32));

    CODEC_FORMAT_CONTEXT *formatCtx;
    formatCtx = GetCodecFormatContext(formatPcm44100c2_24in32);
    ASSERT(formatCtx);

    UNREFERENCED_PARAMETER(formatCtx);

    // PCM:48000 channel:2 24in32
    ACX_DATAFORMAT_CONFIG_INIT_KS(&formatCfg, &Pcm48000c2_24in32);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_FORMAT_CONTEXT);
    attributes.ParentObject = circuit;

    ACXDATAFORMAT formatPcm48000c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatCreate(Device, &attributes, &formatCfg, &formatPcm48000c2_24in32));

    formatCtx = GetCodecFormatContext(formatPcm48000c2_24in32);
    ASSERT(formatCtx);
    UNREFERENCED_PARAMETER(formatCtx);

    ///////////////////////////////////////////////////////////
    //
    // Create render pin. AcxCircuit creates the other pin by default.
    //

    ACX_PIN_CALLBACKS pinCallbacks;
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat = CodecR_EvtAcxPinSetDataFormat;

    ACX_PIN_CONFIG pinCfg;
    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = AcxPinTypeSink;
    pinCfg.Communication = AcxPinCommunicationNone;
    pinCfg.Category = &KSCATEGORY_AUDIO;
    pinCfg.PinCallbacks = &pinCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_PIN_CONTEXT);
    attributes.EvtCleanupCallback = CodecR_EvtPinContextCleanup;
    attributes.ParentObject = circuit;

    ACXPIN pin;
    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(circuit, &attributes, &pinCfg, &pin));

    ASSERT(pin != NULL);
    CODEC_PIN_CONTEXT *pinCtx;
    pinCtx = GetCodecPinContext(pin);
    ASSERT(pinCtx);

    //
    // Add our supported formats to the Default mode for the circuit
    //
    ACXDATAFORMATLIST formatList;
    formatList = AcxPinGetRawDataFormatList(pin);
    if (formatList == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    RETURN_NTSTATUS_IF_FAILED(status);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm48000c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32));

    // Add render pin, using default pin id (0) 
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    ///////////////////////////////////////////////////////////
    //
    // Create Bridge Pin.
    //

    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = AcxPinTypeSource;
    pinCfg.Communication = AcxPinCommunicationNone;
    pinCfg.Category = &KSNODETYPE_SPEAKER;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_PIN_CONTEXT);
    attributes.EvtCleanupCallback = CodecR_EvtPinContextCleanup;
    attributes.ParentObject = circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(circuit, &attributes, &pinCfg, &pin));

    ASSERT(pin != NULL);
    pinCtx = GetCodecPinContext(pin);
    ASSERT(pinCtx);

    RETURN_NTSTATUS_IF_FAILED(AddJack(attributes, pin, SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, RGB(0, 0, 0), AcxConnTypeAtapiInternal, AcxGeoLocFront, AcxGenLocPrimaryBox, AcxPortConnIntegratedDevice));

    // Add render bridge pin
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    ConnectRenderCircuitElements(numElements, elements, circuit);

    //
    // Set output value.
    //
    *Circuit = circuit;

    //
    // Done. 
    //
    status = STATUS_SUCCESS;


    return status;
}

_Use_decl_annotations_
#pragma code_seg()
NTSTATUS
CodecR_EvtCircuitPowerUp (
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PreviousState);

    CODEC_RENDER_CIRCUIT_CONTEXT *  circuitCtx;
    CODEC_MUTE_ELEMENT_CONTEXT *    muteCtx;
    CODEC_VOLUME_ELEMENT_CONTEXT *  volumeCtx;

    // for testing. 
    circuitCtx = GetRenderCircuitContext(Circuit);
    ASSERT(circuitCtx);

    ASSERT(circuitCtx->MuteElement);
    muteCtx = GetCodecMuteElementContext(circuitCtx->MuteElement);
    ASSERT(muteCtx);

    ASSERT(circuitCtx->VolumeElement);
    volumeCtx = GetCodecVolumeElementContext(circuitCtx->VolumeElement);
    ASSERT(volumeCtx);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CodecR_EvtCircuitPowerDown (
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(TargetState);

    CODEC_RENDER_CIRCUIT_CONTEXT *  circuitCtx;
    CODEC_MUTE_ELEMENT_CONTEXT *    muteCtx;
    CODEC_VOLUME_ELEMENT_CONTEXT *  volumeCtx;

    PAGED_CODE();

    // for testing. 
    circuitCtx = GetRenderCircuitContext(Circuit);
    ASSERT(circuitCtx);

    ASSERT(circuitCtx->MuteElement);
    muteCtx = GetCodecMuteElementContext(circuitCtx->MuteElement);
    ASSERT(muteCtx);

    ASSERT(circuitCtx->VolumeElement);
    volumeCtx = GetCodecVolumeElementContext(circuitCtx->VolumeElement);
    ASSERT(volumeCtx);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CodecR_EvtCircuitCompositeCircuitInitialize(
    _In_     WDFDEVICE      Device,
    _In_     ACXCIRCUIT     Circuit,
    _In_opt_ ACXOBJECTBAG   CircuitProperties
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);

    NTSTATUS status = STATUS_SUCCESS;

    if (CircuitProperties != NULL)
    {
        DECLARE_CONST_ACXOBJECTBAG_DRIVER_PROPERTY_NAME(msft, TestUI4);
        ULONG testUI4 = 0;

        status = AcxObjectBagRetrieveUI4(CircuitProperties, &TestUI4, &testUI4);
    }

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
CodecR_EvtCircuitCompositeInitialize(
    _In_ WDFDEVICE      Device,
    _In_ ACXCIRCUIT     Circuit,
    _In_ ACXOBJECTBAG   CompositeProperties
    )
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);

    ASSERT(CompositeProperties);

    DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(UniqueID);
    GUID uniqueId = {0};
    status = AcxObjectBagRetrieveGuid(CompositeProperties, &UniqueID, &uniqueId);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
CodecR_EvtCircuitCreateStream(
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

    DrvLogEnter(g_SDCAVCodecLog);

    UNREFERENCED_PARAMETER(Pin);
    UNREFERENCED_PARAMETER(SignalProcessingMode);

    ASSERT(IsEqualGUID(*SignalProcessingMode, AUDIO_SIGNALPROCESSINGMODE_RAW));

    PCODEC_RENDER_DEVICE_CONTEXT devCtx;
    devCtx = GetRenderDeviceContext(Device);
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
                                            CodecR_EvtStreamRequestPreprocess,
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
    streamCallbacks.EvtAcxStreamAssignDrmContentId  = Codec_EvtStreamAssignDrmContentId;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxStreamCallbacks(StreamInit, &streamCallbacks));

    //
    // Create the stream.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_STREAM_CONTEXT);
    attributes.EvtDestroyCallback = Codec_EvtStreamDestroy;
    ACXSTREAM stream;
    RETURN_NTSTATUS_IF_FAILED(AcxStreamCreate(Device, Circuit, &attributes, &StreamInit, &stream));

    CRenderStreamEngine *streamEngine = NULL;
    streamEngine = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CRenderStreamEngine(stream, StreamFormat);
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


