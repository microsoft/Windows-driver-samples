/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    capture.cpp

Abstract:

    capture factory and circuit

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
#include "CircuitHelper.h"

#include "TestProperties.h"
#include "KeywordDetector.h"
#include "sdcastreaming.h"

#include "AudioFormats.h"

#ifndef __INTELLISENSE__
#include "capture.tmh"
#endif

//
// max # of streams.
//
#define DSPC_MAX_OUTPUT_SYSTEM_STREAMS          1
#define DSPC_MAX_OUTPUT_KEYWORDDETECTOR_STREAMS 1

//
// Factory circuit IDs.
//
#define CAPTURE_DEVICE_ID_STR L"{4DCB0606-6415-4A36-BDC5-9B1792117DC9}\\Capture&CP_%wZ"
DECLARE_CONST_UNICODE_STRING(CaptureHardwareId, L"{4DCB0606-6415-4A36-BDC5-9B1792117DC9}\\Capture");

PAGED_CODE_SEG
NTSTATUS DspC_EvtAcxKeywordSpotterRetrieveArm(
    _In_    ACXKEYWORDSPOTTER   KeywordSpotter,
    _In_    GUID *              EventId,
    _Out_   BOOLEAN *           Arm
)
{
    PAGED_CODE();
    PDSP_KEYWORDSPOTTER_CONTEXT keywordSpotterCtx;
    CKeywordDetector *          keywordDetector = NULL;

    keywordSpotterCtx = GetDspKeywordSpotterContext(KeywordSpotter);

    keywordDetector = (CKeywordDetector*)keywordSpotterCtx->KeywordDetector;

    RETURN_NTSTATUS_IF_FAILED(keywordDetector->GetArmed(*EventId, Arm));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS DspC_EvtAcxKeywordSpotterAssignArm(
    _In_    ACXKEYWORDSPOTTER   KeywordSpotter,
    _In_    GUID *              EventId,
    _In_    BOOLEAN             Arm
)
{
    PAGED_CODE();
    PDSP_KEYWORDSPOTTER_CONTEXT     keywordSpotterCtx;
    CKeywordDetector *              keywordDetector = NULL;

    keywordSpotterCtx = GetDspKeywordSpotterContext(KeywordSpotter);

    keywordDetector = (CKeywordDetector*)keywordSpotterCtx->KeywordDetector;

    RETURN_NTSTATUS_IF_FAILED(keywordDetector->SetArmed(*EventId, Arm));

    // the following code is for example only, after arming the
    // requested keyword we immediately trigger a detection
    // so that the automated tests do not block.
    if (Arm)
    {
        CONTOSO_KEYWORDDETECTIONRESULT detectionResult;

        // notify the keyword detector that we have a notification, to populate
        // timestamp information for this detection.
        keywordDetector->NotifyDetection();

        // fill in the detection specific information
        detectionResult.EventId = *EventId;
        detectionResult.Header.Size = sizeof(CONTOSO_KEYWORDDETECTIONRESULT);
        detectionResult.Header.PatternType = CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER2;
        detectionResult.KeywordStartTimestamp = keywordDetector->GetStartTimestamp();
        detectionResult.KeywordStopTimestamp = keywordDetector->GetStopTimestamp();
        keywordDetector->GetDetectorData(*EventId, &(detectionResult.ContosoDetectorResultData));

        RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(keywordSpotterCtx->Event, &detectionResult, sizeof(CONTOSO_KEYWORDDETECTIONRESULT)));
    }

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS DspC_EvtAcxKeywordSpotterAssignPatterns(
    _In_    ACXKEYWORDSPOTTER   KeywordSpotter,
    _In_    GUID *              EventId,
    _In_    PVOID               Pattern,
    _In_    ULONG               PatternSize
    )
{
    KSMULTIPLE_ITEM *               itemsHeader = nullptr;
    SOUNDDETECTOR_PATTERNHEADER *   patternHeader;
    CONTOSO_KEYWORDCONFIGURATION *  pattern;
    ULONG                           cbRemaining = 0;
    PDSP_KEYWORDSPOTTER_CONTEXT     keywordSpotterCtx;
    CKeywordDetector *              keywordDetector = NULL;

    PAGED_CODE();

    keywordSpotterCtx = GetDspKeywordSpotterContext(KeywordSpotter);

    keywordDetector = (CKeywordDetector*)keywordSpotterCtx->KeywordDetector;

    cbRemaining = PatternSize;

    // The SYSVADPROPERTY_ITEM for this property ensures the value size is at
    // least sizeof KSMULTIPLE_ITEM.
    RETURN_NTSTATUS_IF_TRUE(cbRemaining < sizeof(KSMULTIPLE_ITEM), STATUS_INVALID_PARAMETER);

    itemsHeader = (KSMULTIPLE_ITEM*)Pattern;

    // Verify property value is large enough to include the items
    RETURN_NTSTATUS_IF_TRUE(itemsHeader->Size > cbRemaining, STATUS_INVALID_PARAMETER);

    // No items so clear the configuration.
    if (itemsHeader->Count == 0)
    {
        keywordDetector->ResetDetector(*EventId);
    }
    else
    {
        // This sample supports only 1 pattern type.
        RETURN_NTSTATUS_IF_TRUE(itemsHeader->Count > 1, STATUS_NOT_SUPPORTED);

        // Bytes remaining after the items header
        cbRemaining = itemsHeader->Size - sizeof(*itemsHeader);

        // Verify the property value is large enough to include the pattern header.
        RETURN_NTSTATUS_IF_TRUE(cbRemaining < sizeof(SOUNDDETECTOR_PATTERNHEADER), STATUS_INVALID_PARAMETER);

        patternHeader = (SOUNDDETECTOR_PATTERNHEADER*)(itemsHeader + 1);

        // Verify the pattern type is supported.
        RETURN_NTSTATUS_IF_TRUE(patternHeader->PatternType != CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER2, STATUS_NOT_SUPPORTED);

        // Verify the property value is large enough for the pattern.
        RETURN_NTSTATUS_IF_TRUE(cbRemaining < patternHeader->Size, STATUS_INVALID_PARAMETER);

        // Verify the pattern is large enough.
        RETURN_NTSTATUS_IF_TRUE(patternHeader->Size != sizeof(CONTOSO_KEYWORDCONFIGURATION), STATUS_INVALID_PARAMETER);

        pattern = (CONTOSO_KEYWORDCONFIGURATION*)(patternHeader);

        RETURN_NTSTATUS_IF_FAILED(keywordDetector->DownloadDetectorData(*EventId, pattern->ContosoDetectorConfigurationData));
    }

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS DspC_EvtAcxKeywordSpotterAssignReset(
    _In_    ACXKEYWORDSPOTTER   KeywordSpotter,
    _In_    GUID *              EventId
    )
{
    PAGED_CODE();
    PDSP_KEYWORDSPOTTER_CONTEXT keywordSpotterCtx;
    CKeywordDetector *            keywordDetector = NULL;

    keywordSpotterCtx = GetDspKeywordSpotterContext(KeywordSpotter);

    keywordDetector = (CKeywordDetector*)keywordSpotterCtx->KeywordDetector;

    RETURN_NTSTATUS_IF_FAILED(keywordDetector->ResetDetector(*EventId));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS DspC_EvtAcxFactoryCircuitCreateCircuitDevice(
    _In_  WDFDEVICE                         Parent,
    _In_  ACXFACTORYCIRCUIT                 Factory,
    _In_  PACX_FACTORY_CIRCUIT_ADD_CIRCUIT  CircuitConfig,
    _Out_ WDFDEVICE *                       Device
)
{
    PAGED_CODE();

    DrvLogEnter(g_SDCAVDspLog);

    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;

    UNREFERENCED_PARAMETER(Factory);

    *Device = NULL;

    // Allocate a generic buffer to hold a PnP ID of this device.
    // MAX_DEVICE_ID_LEN is the count of wchar in the device ID name.
    C_ASSERT(NTSTRSAFE_UNICODE_STRING_MAX_CCH >= MAX_DEVICE_ID_LEN);
    C_ASSERT(USHORT_MAX >= MAX_DEVICE_ID_LEN * sizeof(WCHAR));
    WCHAR *wstrBuffer = NULL;
    const USHORT wstrBufferCch = MAX_DEVICE_ID_LEN;
    wstrBuffer = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) WCHAR[wstrBufferCch];
    RETURN_NTSTATUS_IF_TRUE(wstrBuffer == NULL, STATUS_INSUFFICIENT_RESOURCES);
    auto wstrBufferFree = scope_exit([&wstrBuffer]() {
        delete[] wstrBuffer;
        wstrBuffer = NULL;
        });

    RtlZeroMemory(wstrBuffer, sizeof(WCHAR) * wstrBufferCch);

    //
    // Create a child audio device for this circuit.
    //
    PWDFDEVICE_INIT devInit = NULL;
    devInit = WdfPdoInitAllocate(Parent);
    RETURN_NTSTATUS_IF_TRUE(NULL == devInit, STATUS_INSUFFICIENT_RESOURCES);
    auto devInitFree = scope_exit([&devInit]() {
        WdfDeviceInitFree(devInit);
        devInit = NULL;
        });

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //

    //
    // Create the PnP Device ID.
    //
    // Retrieve the unique id of this composite. This logic uses this unique id to 
    // make the device id unique. Using a deterministic value for the pnp device id, guarantees 
    // that the KS properties associated with this audio device interface stay the same across 
    // reboots, even when the circuit factory is used in several ACX composites.
    //
    {
        DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(UniqueID);

        ACX_OBJECTBAG_CONFIG objBagCfg;
        ACX_OBJECTBAG_CONFIG_INIT(&objBagCfg);
        objBagCfg.Handle = CircuitConfig->CompositeProperties;
        objBagCfg.Flags |= AcxObjectBagConfigOpenWithHandle;

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        ACXOBJECTBAG objBag = NULL;
        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagOpen(&attributes, &objBagCfg, &objBag));
        auto objBagFree = scope_exit([&objBag]() {
            WdfObjectDelete(objBag);
            objBag = NULL;
            });

        GUID uniqueId = { 0 };
        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagRetrieveGuid(objBag, &UniqueID, &uniqueId));

        UNICODE_STRING uniqueIdStr = { 0 };
        RETURN_NTSTATUS_IF_FAILED(RtlStringFromGUID(uniqueId, &uniqueIdStr));

        // Init the deviceId unicode string.
        UNICODE_STRING pnpDeviceId = {0};
        pnpDeviceId.Buffer = wstrBuffer;
        pnpDeviceId.Length = 0;
        pnpDeviceId.MaximumLength = (USHORT)(sizeof(WCHAR) * wstrBufferCch);

        status = RtlUnicodeStringPrintf(&pnpDeviceId, CAPTURE_DEVICE_ID_STR, &uniqueIdStr);

        RtlFreeUnicodeString(&uniqueIdStr);

        RETURN_NTSTATUS_IF_FAILED(status);

        // This is the device ID and the first H/W ID. 
        // This ID is used to create a unique audio device interface.
        // Note that this ID is NOT the match with this driver's INF.
        RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignDeviceID(devInit, &pnpDeviceId));

        RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddHardwareID(devInit, &pnpDeviceId));
    }

    // This H/W ID is the match with this driver's INF.
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddHardwareID(devInit, &CaptureHardwareId));

    /*
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
                                     &CaptureDeviceLocation,
                                     &CaptureDeviceLocation,
                                     0x409));
    */

    WdfPdoInitSetDefaultLocale(devInit, 0x409);

    //
    // Allow ACX to add any pre-requirement it needs on this device.
    //
    ACX_DEVICEINIT_CONFIG devInitCfg;
    ACX_DEVICEINIT_CONFIG_INIT(&devInitCfg);
    devInitCfg.Flags |= AcxDeviceInitConfigRawDevice; 
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitInitialize(devInit, &devInitCfg));

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = DspC_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = DspC_EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = DspC_EvtDeviceSelfManagedIoInit;
    WdfDeviceInitSetPnpPowerEventCallbacks(devInit, &pnpPowerCallbacks);

    //
    // Specify a context for this capture device.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_CAPTURE_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = DspC_EvtDeviceContextCleanup;
    attributes.ExecutionLevel = WdfExecutionLevelPassive;

    WDFDEVICE device;
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&devInit, &attributes, &device));
    devInitFree.release();

    //
    // Init capture's device context.
    //
    PDSP_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(device);
    ASSERT(devCtx != NULL);

    //
    // Set device capabilities.
    //
    {
        WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
        WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);

        pnpCaps.SurpriseRemovalOK = WdfTrue;
        pnpCaps.UniqueID = WdfFalse; 

        WdfDeviceSetPnpCapabilities(device, &pnpCaps);
    }

    //
    // Allow ACX to add any post-requirement it needs on this device.
    //
    ACX_DEVICE_CONFIG devCfg;
    ACX_DEVICE_CONFIG_INIT(&devCfg);
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitialize(device, &devCfg));

    *Device = device;

    return status;
}

PAGED_CODE_SEG
NTSTATUS DspC_CreateKeywordSpotterElement(
    _In_    WDFDEVICE           Device,
    _In_    ACXCIRCUIT          Circuit,
    _Out_   ACXKEYWORDSPOTTER * Element
)
{
    WDF_OBJECT_ATTRIBUTES           attributes;
    ACX_KEYWORDSPOTTER_CALLBACKS    keywordSpotterCallbacks;
    ACX_KEYWORDSPOTTER_CONFIG       keywordSpotterCfg;
    PDSP_KEYWORDSPOTTER_CONTEXT     keywordSpotterCtx;
    ACX_PNPEVENT_CONFIG             keywordEventCfg;
    ACXPNPEVENT                     keywordEvent;

    PAGED_CODE();

    ACX_KEYWORDSPOTTER_CALLBACKS_INIT(&keywordSpotterCallbacks);
    keywordSpotterCallbacks.EvtAcxKeywordSpotterRetrieveArm = DspC_EvtAcxKeywordSpotterRetrieveArm;
    keywordSpotterCallbacks.EvtAcxKeywordSpotterAssignArm = DspC_EvtAcxKeywordSpotterAssignArm;
    keywordSpotterCallbacks.EvtAcxKeywordSpotterAssignPatterns = DspC_EvtAcxKeywordSpotterAssignPatterns;
    keywordSpotterCallbacks.EvtAcxKeywordSpotterAssignReset = DspC_EvtAcxKeywordSpotterAssignReset;
    
    ACX_KEYWORDSPOTTER_CONFIG_INIT(&keywordSpotterCfg);
    keywordSpotterCfg.Pattern = &CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER2;
    keywordSpotterCfg.Callbacks = &keywordSpotterCallbacks;
    
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_KEYWORDSPOTTER_CONTEXT);
    attributes.ParentObject = Circuit;
    
    RETURN_NTSTATUS_IF_FAILED(AcxKeywordSpotterCreate(Circuit, &attributes, &keywordSpotterCfg, Element));
    
    keywordSpotterCtx = GetDspKeywordSpotterContext(*Element);
    ASSERT(keywordSpotterCtx);
    
    keywordSpotterCtx->KeywordDetector = (PVOID) new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CKeywordDetector(Device, Circuit, &(Pcm44100c1.WaveFormatExt));
    RETURN_NTSTATUS_IF_TRUE(keywordSpotterCtx->KeywordDetector == NULL, STATUS_INSUFFICIENT_RESOURCES);
    
    ACX_PNPEVENT_CONFIG_INIT(&keywordEventCfg);
    
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PNPEVENT_CONTEXT);
    attributes.ParentObject = *Element;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, *Element, &attributes, &keywordEventCfg, &keywordEvent));

    keywordSpotterCtx->Event = keywordEvent;

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS DspC_EvtAcxFactoryCircuitCreateCircuit(
    _In_ WDFDEVICE                          Parent,
    _In_ WDFDEVICE                          Device,
    _In_ ACXFACTORYCIRCUIT                  Factory,
    _In_ PACX_FACTORY_CIRCUIT_ADD_CIRCUIT   CircuitConfig,
    _In_ PACXCIRCUIT_INIT                   CircuitInit,
    _In_ ULONG                              DataPortNumber,
    _In_ PSDCA_PATH_DESCRIPTORS2            PathDescriptors
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Factory);

    DrvLogEnter(g_SDCAVDspLog);

    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;

    DECLARE_CONST_UNICODE_STRING(circuitName, L"Microphone0");

    //
    // Init output value.
    //
    ASSERT(Device);

    DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(EndpointId);
    ULONG endpointId = 0;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    RETURN_NTSTATUS_IF_FAILED(RetrieveProperties(CircuitConfig, &endpointId));

    ///////////////////////////////////////////////////////////
    //
    // Create a circuit.
    //

    ACXCIRCUIT circuit;
    RETURN_NTSTATUS_IF_FAILED(CreateCaptureCircuit(CircuitInit, circuitName, Device, &circuit));

    AcpiReader * acpiReader = GetAcpiReaderDeviceContext(Parent);
    RETURN_NTSTATUS_IF_FAILED(DetermineSpecialStreamDetailsFromVendorProperties(circuit, acpiReader, CircuitConfig->CircuitProperties));

    ASSERT(circuit != NULL);
    DSP_CIRCUIT_CONTEXT *circuitCtx;
    circuitCtx = GetDspCircuitContext(circuit);
    ASSERT(circuitCtx);

    circuitCtx->EndpointId = endpointId;
    circuitCtx->DataPortNumber = DataPortNumber;

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
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;

    const int numElements = 3;
    ACXELEMENT elements[numElements] = {0};

    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(circuit, &attributes, &elementCfg, &elements[0]));

    ASSERT(elements[0] != NULL);
    DSP_ELEMENT_CONTEXT *elementCtx;
    elementCtx = GetDspElementContext(elements[0]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Create 2nd custom circuit-element.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(circuit, &attributes, &elementCfg, &elements[1]));

    ASSERT(elements[1] != NULL);
    elementCtx = GetDspElementContext(elements[1]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Create 3rd circuit-element, keyword spotter
    //
    RETURN_NTSTATUS_IF_FAILED(DspC_CreateKeywordSpotterElement(Device, circuit, (ACXKEYWORDSPOTTER *) &elements[2]));
    circuitCtx->KeywordSpotter = (ACXKEYWORDSPOTTER)elements[2];

    //
    // Add the circuit elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(circuit, elements, SIZEOF_ARRAY(elements)));

    ///////////////////////////////////////////////////////////
    //
    // Allocate the formats this circuit supports.
    //
    // PCM:44100 channel:2 24in32
    ACXDATAFORMAT formatPcm44100c2nomask;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm44100c2_24in32_nomask, circuit, Device, &formatPcm44100c2nomask));

    // PCM:48000 channel:2 24in32 (Needed for SDCA class driver bring up)
    // The No-Mask version matches what real drivers use for multi-channel capture
    ACXDATAFORMAT formatPcm48000c2nomask;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm48000c2_24in32_nomask, circuit, Device, &formatPcm48000c2nomask));

    // PCM:16000 channel:4 - this is used solely for the KeywordSpotterPin
    // The No-Mask version matches what real drivers use for multi-channel capture
    ACXDATAFORMAT formatPcm16000c4nomask;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm16000c4nomask, circuit, Device, &formatPcm16000c4nomask));

    ///////////////////////////////////////////////////////////
    //
    // Create capture pin. AcxCircuit creates the other pin by default.
    //
    ACXPIN pin;
    ACX_PIN_CALLBACKS pinCallbacks;

    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat  = DspC_EvtAcxPinSetDataFormat;

    RETURN_NTSTATUS_IF_FAILED(CreatePin(AcxPinTypeSource,
                                        circuit,
                                        AcxPinCommunicationSink,
                                        &KSCATEGORY_AUDIO,
                                        &pinCallbacks,
                                        DSPC_MAX_OUTPUT_SYSTEM_STREAMS,
                                        false,
                                        &pin));
    ASSERT(pin != NULL);

    DSP_PIN_CONTEXT* pinCtx;
    pinCtx = GetDspPinContext(pin);
    ASSERT(pinCtx);
    pinCtx->CapturePinType = DspCapturePinTypeHost;

    //
    // Don't add any supported formats here, those will be added when this circuit
    // connects to the downstream circuit
    //

    //
    // Add capture pin, using default pin id (0)
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    ///////////////////////////////////////////////////////////
    //
    // Create keyword streaming pin.
    //
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat  = DspC_EvtAcxPinSetDataFormat;


    pin = NULL;
    RETURN_NTSTATUS_IF_FAILED(CreatePin(AcxPinTypeSource,
                                        circuit,
                                        AcxPinCommunicationSink,
                                        &KSNODETYPE_AUDIO_KEYWORDDETECTOR,
                                        &pinCallbacks,
                                        DSPC_MAX_OUTPUT_KEYWORDDETECTOR_STREAMS,
                                        true,
                                        &pin));

    ASSERT(pin != NULL);
    pinCtx = GetDspPinContext(pin);
    ASSERT(pinCtx);
    pinCtx->CapturePinType = DspCapturePinTypeKeyword;

     //
    // Add our supported formats to the raw mode for the circuit
    //
    ACXDATAFORMATLIST formatList = AcxPinGetRawDataFormatList(pin);
    RETURN_NTSTATUS_IF_TRUE(NULL == formatList, STATUS_INSUFFICIENT_RESOURCES);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm16000c4nomask));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    ///////////////////////////////////////////////////////////
    //
    // Create bridge pin.
    //
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinConnected = DspC_EvtPinConnected;
    pinCallbacks.EvtAcxPinDisconnected = DspC_EvtPinDisconnected;

    pin = NULL;
    RETURN_NTSTATUS_IF_FAILED(CreatePin(AcxPinTypeSink,
                                        circuit,
                                        AcxPinCommunicationNone,
                                        &KSCATEGORY_AUDIO,
                                        &pinCallbacks,
                                        0, // max streams.
                                        false,
                                        &pin));

    ASSERT(pin != NULL);
    pinCtx = GetDspPinContext(pin);
    ASSERT(pinCtx);
    pinCtx->CapturePinType = DspCapturePinTypeBridge;

    //
    // Add a stream BRIDGE.
    //

    ACX_STREAM_BRIDGE_CONFIG streamCfg;
    ACX_STREAM_BRIDGE_CONFIG_INIT(&streamCfg);
    RETURN_NTSTATUS_IF_FAILED(CreateStreamBridge(streamCfg, circuit, pin, pinCtx,  DataPortNumber,  endpointId, PathDescriptors, false));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    RETURN_NTSTATUS_IF_FAILED(ConnectCaptureCircuitElements(3, elements, circuit));

    //
    // Store the circuit handle in the capture device context.
    //
    PDSP_CAPTURE_DEVICE_CONTEXT captureDevCtx = NULL;
    captureDevCtx = GetCaptureDeviceContext(Device);
    ASSERT(captureDevCtx);
    captureDevCtx->Circuit = circuit;
    captureDevCtx->FirstTimePrepareHardware = TRUE;

    return status;
}

//
// This callback is called when the Circuit bridge pin is connected to
// bridge pin of another circuit.
//
// This will happen when the composite circuit is fully initialized.
// From this point onwards the TargetCircuit can be used to send
// KSPROPERTY requests
//
// params:
// TargetCircuit - ACX wrapper for WDFIOTARGET for the connected circuit
// TargetPinId - The pin on the connected circuit. This can be used to
//               send pin specific KSPROPERTY requests.
//
PAGED_CODE_SEG
VOID
DspC_EvtPinConnected (
    _In_    ACXPIN           Pin,
    _In_    ACXTARGETCIRCUIT TargetCircuit,
    _In_    ULONG            TargetPinId
    )
{
    PAGED_CODE();

    DSP_PIN_CONTEXT *pinCtx;
    pinCtx = GetDspPinContext(Pin);
    pinCtx->TargetCircuit = TargetCircuit;
    pinCtx->TargetPinId = TargetPinId;

    // For this sample driver, we're only adding formats to the host pin that the downstream
    // pin supports. For a real DSP driver, the AUDIO_SIGNALPROCESSINGMODE_RAW data format list
    // would probably include all the downstream formats, but the _DEFAULT and possibly _SPEECH
    // or _COMMUNICATIONS modes would contain different formats.
    // As an example, a Microphone Array's _RAW mode formats should match the channel count of the
    // number of microphones in the array, whereas the _DEFAULT mode formats would be the processed
    // stream in Stereo.
    NTSTATUS status;
    ACXPIN hostPin = AcxCircuitGetPinById(AcxPinGetCircuit(Pin), DspCapturePinTypeHost);
    status = ReplicateFormatsForPin(hostPin, TargetCircuit, TargetPinId);
    if (!NT_SUCCESS(status))
    {
        DrvLogError(g_SDCAVDspLog, FLAG_STREAM, L"Failed to replicate downstream formats to host pin, %!STATUS!",
            status);
    }

    // The ACX Framework will maintain the TargetCircuit until after it's called EvtPinDisconnect.
}

//
// This callback is called when the Circuit bridge pin is disconnected
// from the bridge pin of another circuit.
//
// This will happen when the composite circuit is deinitialized.
// From this point onwards the TargetCircuit cannnot be used to send
// KSPROPERTY requests.
// TargetCircuit should only be used to access the attached context.
//
// params:
// TargetCircuit - ACX wrapper for WDFIOTARGET for the connected circuit
// TargetPinId - The pin on the connected circuit.
//
PAGED_CODE_SEG
VOID
DspC_EvtPinDisconnected (
    _In_    ACXPIN           Pin,
    _In_    ACXTARGETCIRCUIT TargetCircuit,
    _In_    ULONG            TargetPinId
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(TargetPinId);
    UNREFERENCED_PARAMETER(TargetCircuit);

    DSP_PIN_CONTEXT *pinCtx;
    pinCtx = GetDspPinContext(Pin);

    if (pinCtx->TargetCircuit)
    {
        // After calling EvtPinDisconnected, the ACX framework will clean up
        // the TargetCircuit.
        pinCtx->TargetCircuit = NULL;
        pinCtx->TargetPinId = (ULONG)(-1);
    }
}

PAGED_CODE_SEG
NTSTATUS DspC_EvtDevicePrepareHardware(
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

    DrvLogEnter(g_SDCAVDspLog);

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    PDSP_CAPTURE_DEVICE_CONTEXT devCtx;
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
        status = STATUS_SUCCESS;
        return status;
    }

    //
    // Set child's power policy.
    //
    RETURN_NTSTATUS_IF_FAILED(DspC_SetPowerPolicy(Device));

    //
    // Add circuit to child's list.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddCircuit(Device, devCtx->Circuit));

    //
    // Keep track this is not the first time this callback was called.
    //
    devCtx->FirstTimePrepareHardware = FALSE;

    DrvLogExit(g_SDCAVDspLog);

    return status;
}

PAGED_CODE_SEG
NTSTATUS DspC_EvtDeviceReleaseHardware(
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
    DrvLogEnter(g_SDCAVDspLog);

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    PDSP_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(Device);
    ASSERT(devCtx != NULL);

    DrvLogExit(g_SDCAVDspLog);

    return status;
}

PAGED_CODE_SEG
NTSTATUS DspC_EvtDeviceSelfManagedIoInit(
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

    PDSP_CAPTURE_DEVICE_CONTEXT devCtx;
    devCtx = GetCaptureDeviceContext(Device);
    ASSERT(devCtx != NULL);

    return STATUS_SUCCESS;
}

#pragma code_seg()
VOID DspC_EvtDeviceContextCleanup(
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
    WDFDEVICE                       device;
    PDSP_CAPTURE_DEVICE_CONTEXT      devCtx;

    device = (WDFDEVICE)WdfDevice;
    devCtx = GetCaptureDeviceContext(device);
    ASSERT(devCtx != NULL);

    // only clean up the circuit if it was
    // successfully created, else it'll crash
    if (devCtx->Circuit != NULL)
    {
        DspC_CircuitCleanup(devCtx->Circuit);
        devCtx->Circuit = NULL;
    }
}

#pragma code_seg()
VOID
DspC_EvtCircuitContextCleanup(
    _In_ WDFOBJECT      Circuit
   )
/*++

Routine Description:

    In this callback, it cleans up circuit context.

Arguments:

    WdfDevice - WDF device object

Return Value:

    NULL

--*/
{
    PDSP_CIRCUIT_CONTEXT circuitCtx;

    circuitCtx = GetDspCircuitContext(Circuit);
    ASSERT(circuitCtx != NULL);

    // clean up the path context information in case it wasn't cleaned up
    // by pin disconnection.
    circuitCtx->SpecialStreamAvailablePaths = 0;

    for(ULONG i = (UINT) SpecialStreamTypeUltrasoundRender; i < (UINT) SpecialStreamType_Count; i++)
    {
        if (circuitCtx->SpecialStreamPathDescriptors[i])
        {
            ExFreePool(circuitCtx->SpecialStreamPathDescriptors[i]);
            circuitCtx->SpecialStreamPathDescriptors[i] = nullptr;
        }
    }

    for (ULONG i = (UINT)SpecialStreamTypeUltrasoundRender; i < (UINT)SpecialStreamType_Count; i++)
    {
        if (circuitCtx->SpecialStreamPathDescriptors2[i])
        {
            ExFreePool(circuitCtx->SpecialStreamPathDescriptors2[i]);
            circuitCtx->SpecialStreamPathDescriptors2[i] = nullptr;
        }
    }

    if (circuitCtx->SpecialStreamTargetCircuit)
    {
        WdfObjectDereferenceWithTag(circuitCtx->SpecialStreamTargetCircuit, (PVOID)DRIVER_TAG);
        circuitCtx->SpecialStreamTargetCircuit = nullptr;
    }

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCA VDSP Circuit Cleanup %p", Circuit);
}

#pragma code_seg()
_Use_decl_annotations_
NTSTATUS DspC_EvtCircuitPowerUp (
    WDFDEVICE,
    ACXCIRCUIT,
    WDF_POWER_DEVICE_STATE
)
{
    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
_Use_decl_annotations_
NTSTATUS DspC_EvtCircuitPowerDown (
    WDFDEVICE,
    ACXCIRCUIT,
    WDF_POWER_DEVICE_STATE
)
{
    PAGED_CODE();

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS DspC_EvtCircuitCompositeCircuitInitialize(
    WDFDEVICE,
    ACXCIRCUIT,
    ACXOBJECTBAG
)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS DspC_EvtCircuitCompositeInitialize(
    WDFDEVICE,
    ACXCIRCUIT,
    ACXOBJECTBAG  
)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    return status;
}

PAGED_CODE_SEG
VOID DspC_EvtCircuitRequestPreprocess(
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
NTSTATUS
DspC_EvtCircuitCreateStream(
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

    UNREFERENCED_PARAMETER(SignalProcessingMode);
    UNREFERENCED_PARAMETER(VarArguments);

    DrvLogEnter(g_SDCAVDspLog);

    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN kwsStream = FALSE;

    DSP_PIN_CONTEXT * pinCtx;
    pinCtx = GetDspPinContext(Pin);
    ASSERT(pinCtx != NULL);

// See description in private.h
#ifdef ACX_WORKAROUND_ACXPIN_01
    {
        ASSERT(pinCtx->CurrentStreamsCount != (ULONG)-1);
        RETURN_NTSTATUS_IF_TRUE_MSG(
                pinCtx->CurrentStreamsCount >= pinCtx->MaxStreams,
                STATUS_INSUFFICIENT_RESOURCES,
                L"ACXCIRCUIT %p ACXPIN %p cannot create another ACXSTREAM, max count is %d, %!STATUS!",
                Circuit, Pin, pinCtx->MaxStreams, status);
    }
#endif

    //
    // Request a Vendor-Specific property from the Controller
    //
    Dsp_SendVendorSpecificProperties(
        Device,
        Circuit,
        FALSE);

    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxRequestPreprocessCallback(
                                            StreamInit, 
                                            DspC_EvtStreamRequestPreprocess,
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
    streamCallbacks.EvtAcxStreamPrepareHardware         = Dsp_EvtStreamPrepareHardware;
    streamCallbacks.EvtAcxStreamReleaseHardware         = Dsp_EvtStreamReleaseHardware;
    streamCallbacks.EvtAcxStreamRun                     = Dsp_EvtStreamRun;
    streamCallbacks.EvtAcxStreamPause                   = Dsp_EvtStreamPause;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxStreamCallbacks(StreamInit, &streamCallbacks));

    //
    // Init RT streaming callbacks.
    //
    ACX_RT_STREAM_CALLBACKS rtCallbacks;
    ACX_RT_STREAM_CALLBACKS_INIT(&rtCallbacks);
    rtCallbacks.EvtAcxStreamGetHwLatency                = Dsp_EvtStreamGetHwLatency;
    rtCallbacks.EvtAcxStreamAllocateRtPackets           = Dsp_EvtStreamAllocateRtPackets;
    rtCallbacks.EvtAcxStreamFreeRtPackets               = Dsp_EvtStreamFreeRtPackets;
    rtCallbacks.EvtAcxStreamGetCapturePacket            = DspC_EvtStreamGetCapturePacket;
    rtCallbacks.EvtAcxStreamGetCurrentPacket            = Dsp_EvtStreamGetCurrentPacket;
    rtCallbacks.EvtAcxStreamGetPresentationPosition     = Dsp_EvtStreamGetPresentationPosition;    

    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxRtStreamCallbacks(StreamInit, &rtCallbacks));

    //
    // Buffer notifications are supported.
    //
    AcxStreamInitSetAcxRtStreamSupportsNotifications(StreamInit);

    //
    // Create the stream.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    ACXSTREAM stream;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_STREAM_CONTEXT);
    attributes.EvtDestroyCallback = Dsp_EvtStreamContextDestroy;
    attributes.EvtCleanupCallback = Dsp_EvtStreamContextCleanup;

    RETURN_NTSTATUS_IF_FAILED(AcxRtStreamCreate(Device, Circuit, &attributes, &StreamInit, &stream));

    DSP_STREAM_CONTEXT* streamCtx;
    streamCtx = GetDspStreamContext(stream);
    ASSERT(streamCtx);

    streamCtx->CapturePinType = pinCtx->CapturePinType;

    DSP_CIRCUIT_CONTEXT * circuitCtx;
    circuitCtx = GetDspCircuitContext(Circuit);
    ASSERT(circuitCtx != NULL);

    CCaptureStreamEngine *streamEngine = NULL;

    if (pinCtx->CapturePinType == DspCapturePinTypeKeyword)
    {
        PDSP_KEYWORDSPOTTER_CONTEXT keywordSpotterCtx;
        keywordSpotterCtx = GetDspKeywordSpotterContext(circuitCtx->KeywordSpotter);
        ASSERT(keywordSpotterCtx);

        streamEngine = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CBufferedCaptureStreamEngine(stream, StreamFormat, (CKeywordDetector *) keywordSpotterCtx->KeywordDetector);
        kwsStream = TRUE;
    }
    else
    {
        streamEngine = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CCaptureStreamEngine(stream, StreamFormat);
    }

    RETURN_NTSTATUS_IF_TRUE(NULL == streamEngine, STATUS_INSUFFICIENT_RESOURCES);

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
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_ELEMENT_CONTEXT);
    attributes.ParentObject = stream;

    ACXELEMENT elements[2] = { 0 };
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(stream, &attributes, &elementCfg, &elements[0]));

    ASSERT(elements[0] != NULL);
    DSP_ELEMENT_CONTEXT *elementCtx;
    elementCtx = GetDspElementContext(elements[0]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Create 2nd custom stream-elements.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_ELEMENT_CONTEXT);
    attributes.ParentObject = stream;

    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(stream, &attributes, &elementCfg, &elements[1]));

    ASSERT(elements[1] != NULL);
    elementCtx = GetDspElementContext(elements[1]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Add stream elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(stream, elements, SIZEOF_ARRAY(elements)));

    ACXPIN bridgePin = AcxCircuitGetPinById(Circuit, (ULONG)DspCapturePinTypeBridge);
    RETURN_NTSTATUS_IF_TRUE(bridgePin == NULL, STATUS_UNSUCCESSFUL);
    PDSP_PIN_CONTEXT bridgePinCtx = GetDspPinContext(bridgePin);
    if (!kwsStream)
    {
        // KWS Streams are handled in the DSP. Only add non-KWS streams to the StreamBridge, which
        // will forward the stream creation to downlevel circuits (i.e. Xu and Codec drivers)
        RETURN_NTSTATUS_IF_FAILED(AcxStreamBridgeAddStream(bridgePinCtx->HostStreamBridge, stream));
    }

// See description in private.h
#ifdef ACX_WORKAROUND_ACXPIN_01
    {
        ASSERT(pinCtx->CurrentStreamsCount != (ULONG)-1);
        InterlockedIncrement(PLONG(&pinCtx->CurrentStreamsCount));
        streamCtx->StreamIsCounted = TRUE;
    }
#endif

    streamCtx->Pin = Pin;
    WdfObjectReferenceWithTag(Pin, (PVOID)DRIVER_TAG);

    return status;
}

PAGED_CODE_SEG
VOID
DspC_EvtStreamRequestPreprocess(
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

    NTSTATUS                            status = STATUS_NOT_SUPPORTED;
    ACX_REQUEST_PARAMETERS              params;
    ULONG_PTR                           outDataCb = 0;
    PDSP_STREAM_CONTEXT                 streamCtx;

    UNREFERENCED_PARAMETER(DriverContext);

    ASSERT(Object != NULL);
    ASSERT(DriverContext);
    ASSERT(Request);

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    streamCtx = GetDspStreamContext(Object);
    if (streamCtx && streamCtx->CapturePinType == DspCapturePinTypeKeyword)
    {
        if (IsEqualGUID(params.Parameters.Property.Set, KSPROPSETID_RtAudio) &&
            params.Parameters.Property.Id == KSPROPERTY_RTAUDIO_PACKETVREGISTER)
        {
            status = STATUS_NOT_SUPPORTED;
            outDataCb = 0;

            DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"DSP Capture Stream for Keyword Overriding PACKETVREGISTER request, %!STATUS!",
                status);

            WdfRequestCompleteWithInformation(Request, status, outDataCb);
            return;
        }
    }

    //
    // Just give the request back to ACX.
    //
    (VOID)AcxStreamDispatchAcxRequest((ACXSTREAM)Object, Request);
}

PAGED_CODE_SEG
NTSTATUS
DspC_SetPowerPolicy(
    _In_ WDFDEVICE      Device
)
{
    NTSTATUS    status = STATUS_SUCCESS;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    //WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;

    PAGED_CODE();

    //
    // Init the idle policy structure.
    //
    //WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCanWakeFromS0);
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeout = 10000; // 10-sec

    status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
DspC_CircuitCleanup(
    _In_ ACXCIRCUIT Circuit
    )
{
    PDSP_CIRCUIT_CONTEXT        circuitCtx;
    PDSP_KEYWORDSPOTTER_CONTEXT keywordSpotterCtx;
    CKeywordDetector *          keywordDetector = NULL;

    PAGED_CODE();

    // Remove the static capture circuit
    circuitCtx = GetDspCircuitContext(Circuit);
    ASSERT(circuitCtx != NULL);

    keywordSpotterCtx = GetDspKeywordSpotterContext(circuitCtx->KeywordSpotter);
    ASSERT(keywordSpotterCtx != NULL);

    keywordDetector = (CKeywordDetector*)keywordSpotterCtx->KeywordDetector;
    keywordSpotterCtx->KeywordDetector = NULL;
    delete keywordDetector;

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
DspC_EvtAcxPinSetDataFormat(
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
DspC_EvtPinContextCleanup(
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
    DSP_PIN_CONTEXT *pinCtx;
    pinCtx = GetDspPinContext(WdfPin);

    if (pinCtx->TargetCircuit)
    {
        WdfObjectDereferenceWithTag(pinCtx->TargetCircuit, (PVOID)DRIVER_TAG);

        pinCtx->TargetCircuit = NULL;
        pinCtx->TargetPinId = (ULONG)(-1);
    }
}

PAGED_CODE_SEG
NTSTATUS
DspC_EvtStreamGetCapturePacket(
    _In_ ACXSTREAM          Stream,
    _Out_ ULONG* LastCapturePacket,
    _Out_ ULONGLONG* QPCPacketStart,
    _Out_ BOOLEAN* MoreData
)
{
    PDSP_STREAM_CONTEXT    ctx;
    CCaptureStreamEngine* streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = static_cast<CCaptureStreamEngine*>(ctx->StreamEngine);

    return streamEngine->GetCapturePacket(LastCapturePacket, QPCPacketStart, MoreData);
}


