/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CircuitHelper.cpp

Abstract:

   This module contains helper functions for render.cpp and capture.cpp files.

Environment:

    Kernel mode

--*/

#include "private.h"
#include "CircuitHelper.h"
#include "TestProperties.h"
#include "AudioFormats.h"

#ifndef __INTELLISENSE__
#include "CircuitHelper.tmh"
#endif

PAGED_CODE_SEG
NTSTATUS CreateCaptureCircuit(
    _In_ PACXCIRCUIT_INIT                   CircuitInit,
    _In_ UNICODE_STRING                     CircuitName,
    _In_ WDFDEVICE                          Device,
    _Out_ ACXCIRCUIT*                       Circuit
)
{

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    // Circuit Component ID already assigned by the device handler

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignName(CircuitInit, &CircuitName));

    //
    // Add circuit type.
    //
    AcxCircuitInitSetCircuitType(CircuitInit, AcxCircuitTypeCapture);

    //
    // Assign the circuit's pnp-power callbacks.
    //
    {
        ACX_CIRCUIT_PNPPOWER_CALLBACKS  powerCallbacks;
        ACX_CIRCUIT_PNPPOWER_CALLBACKS_INIT(&powerCallbacks);
        powerCallbacks.EvtAcxCircuitPowerUp = DspC_EvtCircuitPowerUp;
        powerCallbacks.EvtAcxCircuitPowerDown = DspC_EvtCircuitPowerDown;
        AcxCircuitInitSetAcxCircuitPnpPowerCallbacks(CircuitInit, &powerCallbacks);
    }

    //
    // Assign the circuit's composite callbacks.
    //
    {
        ACX_CIRCUIT_COMPOSITE_CALLBACKS compositeCallbacks;
        ACX_CIRCUIT_COMPOSITE_CALLBACKS_INIT(&compositeCallbacks);
        compositeCallbacks.EvtAcxCircuitCompositeCircuitInitialize = DspC_EvtCircuitCompositeCircuitInitialize;
        compositeCallbacks.EvtAcxCircuitCompositeInitialize = DspC_EvtCircuitCompositeInitialize;
        AcxCircuitInitSetAcxCircuitCompositeCallbacks(CircuitInit, &compositeCallbacks);
    }


    //
    // Add pre-process callbacks.
    //
// See description in private.h
#ifdef ACX_WORKAROUND_ACXPIN_01
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
                                CircuitInit, 
                                Dsp_EvtStreamGetStreamCountRequestPreprocess,
                                (ACXCONTEXT)Device,
                                AcxRequestTypeProperty,
                                &KSPROPSETID_Pin,
                                KSPROPERTY_PIN_CINSTANCES));
#endif // ACX_WORKAROUND_ACXPIN_01

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
                                CircuitInit,
                                DspC_EvtCircuitRequestPreprocess,
                                (ACXCONTEXT)AcxRequestTypeAny, // dbg only
                                AcxRequestTypeAny,
                                NULL,
                                AcxItemIdNone));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxCreateStreamCallback(
        CircuitInit,
        DspC_EvtCircuitCreateStream));

    /*
    //
    // Add properties, events and methods.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignProperties(CircuitInit,
                                            CircuitProperties,
                                            CircuitPropertiesCount));
    */

    //
    // Create the circuit.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_CIRCUIT_CONTEXT);
    attributes.EvtCleanupCallback = DspC_EvtCircuitContextCleanup;
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitCreate(Device, &attributes, &CircuitInit, Circuit));

    return status;
}

PAGED_CODE_SEG
VOID Dsp_EvtPropertyResourceGroup(
    _In_ ACXOBJECT Circuit,
    _In_ WDFREQUEST Request
)
{
    PAGED_CODE();

    ACX_REQUEST_PARAMETERS params;
    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    PAUDIORESOURCEMANAGEMENT_RESOURCEGROUP resourceGroup =
        (PAUDIORESOURCEMANAGEMENT_RESOURCEGROUP)params.Parameters.Property.Value;

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCA VDSP Circuit %p received KSPROPERTY_AUDIORESOURCEMANAGEMENT_RESOURCEGROUP with group \"%ls\" %ls",
        Circuit, resourceGroup->ResourceGroupName, resourceGroup->ResourceGroupAcquired ? L"Acquired" : L"Released");

    WdfRequestComplete(Request, STATUS_SUCCESS);
}


PAGED_CODE_SEG
NTSTATUS CreateRenderCircuit(
    _In_ PACXCIRCUIT_INIT                   CircuitInit,
    _In_ UNICODE_STRING                     CircuitName,
    _In_ WDFDEVICE                          Device,
    _Out_ ACXCIRCUIT*                       Circuit
)
{

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    // Circuit Component ID already assigned by the device handler

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignName(CircuitInit, &CircuitName));

    //
    // Add circuit type.
    //
    AcxCircuitInitSetCircuitType(CircuitInit, AcxCircuitTypeRender);

    //
    // Assign the circuit's pnp-power callbacks.
    //
    {
        ACX_CIRCUIT_PNPPOWER_CALLBACKS  powerCallbacks;
        ACX_CIRCUIT_PNPPOWER_CALLBACKS_INIT(&powerCallbacks);
        powerCallbacks.EvtAcxCircuitPowerUp = DspR_EvtCircuitPowerUp;
        powerCallbacks.EvtAcxCircuitPowerDown = DspR_EvtCircuitPowerDown;
        AcxCircuitInitSetAcxCircuitPnpPowerCallbacks(CircuitInit, &powerCallbacks);
    }

    //
    // Assign the circuit's composite callbacks.
    //
    {
        ACX_CIRCUIT_COMPOSITE_CALLBACKS compositeCallbacks;
        ACX_CIRCUIT_COMPOSITE_CALLBACKS_INIT(&compositeCallbacks);
        compositeCallbacks.EvtAcxCircuitCompositeCircuitInitialize = DspR_EvtCircuitCompositeCircuitInitialize;
        compositeCallbacks.EvtAcxCircuitCompositeInitialize = DspR_EvtCircuitCompositeInitialize;
        AcxCircuitInitSetAcxCircuitCompositeCallbacks(CircuitInit, &compositeCallbacks);
    }

    //
    // Assign properties handled by the circuit.
    //
    {
        ACX_PROPERTY_ITEM RenderCircuitProperties[] =
        {
            {
                &KSPROPSETID_AudioResourceManagement,
                KSPROPERTY_AUDIORESOURCEMANAGEMENT_RESOURCEGROUP,
                ACX_PROPERTY_ITEM_FLAG_SET,
                Dsp_EvtPropertyResourceGroup,
                nullptr,
                0,
                sizeof(AUDIORESOURCEMANAGEMENT_RESOURCEGROUP),
                0
            },
        };

        RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignProperties(CircuitInit, RenderCircuitProperties, ARRAYSIZE(RenderCircuitProperties)));
    }
    //
    // Add pre-process callbacks.
    //
// See description in private.h
#ifdef ACX_WORKAROUND_ACXPIN_01
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
                                CircuitInit, 
                                Dsp_EvtStreamGetStreamCountRequestPreprocess,
                                (ACXCONTEXT)Device,
                                AcxRequestTypeProperty,
                                &KSPROPSETID_Pin,
                                KSPROPERTY_PIN_CINSTANCES));
#endif

// See description in private.h
#ifdef ACX_WORKAROUND_ACXPIN_02
        RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
                                    CircuitInit, 
                                    Dsp_EvtStreamProposeDataFormatRequestPreprocess,
                                    (ACXCONTEXT)Device,
                                    AcxRequestTypeProperty,
                                    &KSPROPSETID_Pin,
                                    KSPROPERTY_PIN_PROPOSEDATAFORMAT));
#endif // ACX_WORKAROUND_ACXPIN_02
    
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
                                CircuitInit,
                                DspR_EvtCircuitRequestPreprocess,
                                (ACXCONTEXT)AcxRequestTypeAny, // dbg only
                                AcxRequestTypeAny,
                                NULL,
                                AcxItemIdNone));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxCreateStreamCallback(
        CircuitInit,
        DspR_EvtCircuitCreateStream));

    /*
    //
    // Add properties, events and methods.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignProperties(CircuitInit,
                                            CircuitProperties,
                                            CircuitPropertiesCount));
    */

    //
    // Disable ACX remote stream handling.
    // This is for testing only b/c by creating an explicit stream-bridge below, 
    // the default ACX behavior for stream-bridge is automatically disabled.
    //
    AcxCircuitInitDisableDefaultStreamBridgeHandling(CircuitInit);

    //
    // Create the circuit.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_CIRCUIT_CONTEXT);
    attributes.EvtCleanupCallback = DspR_EvtCircuitContextCleanup;
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitCreate(Device, &attributes, &CircuitInit, Circuit));

    return status;
}

PAGED_CODE_SEG
NTSTATUS AllocateFormat(
    _In_ KSDATAFORMAT_WAVEFORMATEXTENSIBLE      WaveFormat,
    _In_ ACXCIRCUIT                             Circuit,
    _In_ WDFDEVICE                              Device,
    _Out_ ACXDATAFORMAT*                        Format
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    ACX_DATAFORMAT_CONFIG formatCfg;
    ACX_DATAFORMAT_CONFIG_INIT_KS(&formatCfg, &WaveFormat);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_FORMAT_CONTEXT);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatCreate(Device, &attributes, &formatCfg, Format));

    ASSERT((*Format) != NULL);
    DSP_FORMAT_CONTEXT* formatCtx;
    formatCtx = GetDspFormatContext(*Format);
    ASSERT(formatCtx);
    UNREFERENCED_PARAMETER(formatCtx);

    return status;
}

PAGED_CODE_SEG
NTSTATUS CreatePin(
    _In_ ACX_PIN_TYPE                           PinType,
    _In_ ACXCIRCUIT                             Circuit,
    _In_ ACX_PIN_COMMUNICATION                  Communication,
    _In_ const GUID*                            Category,
    _In_ ACX_PIN_CALLBACKS*                     PinCallbacks,
    _In_ ULONG                                  PinStreamCount,
    _In_ bool                                   Mic,
    _Out_ ACXPIN*                               Pin
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    ACX_PIN_CONFIG pinCfg;
    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = PinType;
    pinCfg.Communication = Communication;
    pinCfg.Category = Category;
    pinCfg.PinCallbacks = PinCallbacks;

// See description in private.h
#ifndef ACX_WORKAROUND_ACXPIN_01
    pinCfg->MaxStreams = PinStreamCount;
#endif

    ACX_MICROPHONE_CONFIG       micCfg;
    ACX_INTERLEAVED_AUDIO_FORMAT_INFORMATION InterleavedFormat;

    if (Mic)
    {
        ACX_MICROPHONE_CONFIG_INIT(&micCfg);
        ACX_INTERLEAVED_AUDIO_FORMAT_INFORMATION_INIT(&InterleavedFormat);

        InterleavedFormat.PrimaryChannelCount = 2;
        InterleavedFormat.PrimaryChannelStartPosition = 0;
        InterleavedFormat.PrimaryChannelMask = 0;
        InterleavedFormat.InterleavedChannelCount = 2;
        InterleavedFormat.InterleavedChannelStartPosition = 2;
        InterleavedFormat.InterleavedChannelMask = KSAUDIO_SPEAKER_STEREO;

        micCfg.InterleavedFormat = &InterleavedFormat;

        pinCfg.Flags |= AcxPinConfigMicrophoneConfigSpecified;
        pinCfg.u.MicrophoneConfig = &micCfg;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PIN_CONTEXT);
    attributes.EvtCleanupCallback = DspR_EvtPinContextCleanup;
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(Circuit, &attributes, &pinCfg, Pin));
    ASSERT(Pin != NULL);

// See description in private.h
#ifdef ACX_WORKAROUND_ACXPIN_01
    {
        PDSP_PIN_CONTEXT pinCtx = GetDspPinContext(*Pin);
        pinCtx->MaxStreams = PinStreamCount;
        pinCtx->CurrentStreamsCount = 0;
    }
#endif

    return status;
}

PAGED_CODE_SEG
NTSTATUS RetrieveProperties(
    _In_ PACX_FACTORY_CIRCUIT_ADD_CIRCUIT           CircuitConfig,
    _Out_ PULONG                                    EndpointID
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(EndpointId);
    DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(DataPortNumber);

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    // Create object bag from the CompositeProperties
    ACXOBJECTBAG compositeProperties;
    ACX_OBJECTBAG_CONFIG propConfig;
    ACX_OBJECTBAG_CONFIG_INIT(&propConfig);
    propConfig.Handle = CircuitConfig->CompositeProperties;
    propConfig.Flags |= AcxObjectBagConfigOpenWithHandle;

    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagOpen(&attributes, &propConfig, &compositeProperties));

    auto cleanupCompositeProperties = scope_exit([=]() {
        WdfObjectDelete(compositeProperties);
        }
    );

    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagRetrieveUI4(compositeProperties, &EndpointId, EndpointID));

    return status;
}

PAGED_CODE_SEG
NTSTATUS
DetermineSpecialStreamDetailsFromVendorProperties(
    _In_        ACXCIRCUIT      Circuit,
    _In_        AcpiReader *    Acpi,
    _In_        HANDLE          CircuitPropertiesHandle
    )
{
    DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(VendorPropertiesBlock);
    WDFMEMORY vendorPropertiesBlock = NULL;
    DSP_CIRCUIT_CONTEXT* circuitCtx;
    NTSTATUS status = STATUS_SUCCESS;
    PSDCA_PATH_DESCRIPTORS2 pPathDesc2 = nullptr;

    PAGED_CODE();

    ACX_OBJECTBAG_CONFIG propConfig;
    ACXOBJECTBAG circuitProperties;
    ACX_OBJECTBAG_CONFIG_INIT(&propConfig);
    propConfig.Handle = CircuitPropertiesHandle;
    propConfig.Flags |= AcxObjectBagConfigOpenWithHandle;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagOpen(&attributes, &propConfig, &circuitProperties));

    auto cleanupPropConfig = scope_exit([=]()
    {
        WdfObjectDelete(circuitProperties);
    });

    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagRetrieveBlob(circuitProperties, &VendorPropertiesBlock, NULL, &vendorPropertiesBlock));

    auto cleanup1 = scope_exit([&vendorPropertiesBlock] ()
    {
        if (vendorPropertiesBlock != NULL)
        {
            WdfObjectDelete(vendorPropertiesBlock);
            vendorPropertiesBlock = NULL;
        }
    });

    //
    // The below code would be replaced in a real DSP driver (or modified to use vendor-specific properties)
    //
    circuitCtx = GetDspCircuitContext(Circuit);
    ASSERT(circuitCtx);

    for (ULONG i = (UINT)SpecialStreamTypeUltrasoundRender; i < (UINT)SpecialStreamType_Count; i++)
    {
        SDCA_PATH path = SdcaPathFromSpecialStreamType((SDCA_SPECIALSTREAM_TYPE)i);
        ULONG propertyValue = 0;
        char propertyName[256];

        RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-size", path));

        // Sample driver uses this proeprty to determine whether to use PathDescriptor2 or PathDescriptor
        NTSTATUS tempStatus = Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue);
        if (!NT_SUCCESS(tempStatus))
        {
            // This special stream is either not supported or does not use PathDescriptor2
            continue;
        }

        pPathDesc2 = (PSDCA_PATH_DESCRIPTORS2)ExAllocatePool2(
            POOL_FLAG_NON_PAGED,
            propertyValue,
            DRIVER_TAG);
        if (pPathDesc2 == nullptr)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        auto cleanup2 = scope_exit([&pPathDesc2]()
        {
            if (pPathDesc2 != NULL)
            {
                ExFreePool(pPathDesc2);
                pPathDesc2 = NULL;
            }
        });

        pPathDesc2->Size = propertyValue;
        pPathDesc2->Version = SDCA_PATH_DESCRIPTOR2_VERSION_1;
        pPathDesc2->SdcaPath = path;

        // Since we found one specialstream property, all others are required to be present
        RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-endpoint-id", path));
        RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
        pPathDesc2->EndpointId = propertyValue;

        pPathDesc2->SpecialPathFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-specialpathformat-channels", path));
        RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
        pPathDesc2->SpecialPathFormat.Format.nChannels = (WORD)propertyValue;

        RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-specialpathformat-bits-per-sample", path));
        RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
        pPathDesc2->SpecialPathFormat.Format.wBitsPerSample = (WORD)propertyValue;

        RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-specialpathformat-samples-per-sec", path));
        RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
        pPathDesc2->SpecialPathFormat.Format.nSamplesPerSec = propertyValue;
        pPathDesc2->SpecialPathFormat.Format.nBlockAlign = pPathDesc2->SpecialPathFormat.Format.nChannels * pPathDesc2->SpecialPathFormat.Format.wBitsPerSample;
        pPathDesc2->SpecialPathFormat.Format.nAvgBytesPerSec = pPathDesc2->SpecialPathFormat.Format.nSamplesPerSec * pPathDesc2->SpecialPathFormat.Format.nBlockAlign;
        pPathDesc2->SpecialPathFormat.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

        RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-specialpathformat-valid-bits-per-sample", path));
        RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
        pPathDesc2->SpecialPathFormat.Samples.wValidBitsPerSample = (WORD)propertyValue;

        RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-specialpathformat-channel-mask", path));
        RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
        pPathDesc2->SpecialPathFormat.dwChannelMask = propertyValue;
        pPathDesc2->SpecialPathFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-desc-count", path));
        RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
        pPathDesc2->DescriptorCount = propertyValue;

        for (ULONG j = 0; j < pPathDesc2->DescriptorCount; j++)
        {
            pPathDesc2->Descriptor[j].Size = sizeof(pPathDesc2->Descriptor[0]);
            pPathDesc2->Descriptor[j].Version = SDCA_PATH_DESCRIPTOR2_VERSION_1;


            // In this sample, we are getting the function informaiton id from audio composition data, however, this id is
            // generated at runtime so the real drivers would have information like function number, peripheral id etc. in
            // its composition data and then use that to map it to a function information id by querying down stream circuit.
            RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-desc-0x%x-func-info-id", path, j));
            RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
            pPathDesc2->Descriptor[j].FunctionInformationId = propertyValue;

            RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-desc-0x%x-terminal-id", path, j));
            RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
            pPathDesc2->Descriptor[j].TerminalEntityId = propertyValue;

            RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-desc-0x%x-dp-map", path, j));
            RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
            pPathDesc2->Descriptor[j].DataPortMap = propertyValue;

            // DataPortMap indicates which DPIndex entries are used, in this sample we'll only use
            // a single data port and that will be DPIndex_A.
            pPathDesc2->Descriptor[j].DataPortConfig[0].Size = sizeof(pPathDesc2->Descriptor[0].DataPortConfig);
            pPathDesc2->Descriptor[j].DataPortConfig[0].EndpointId = pPathDesc2->EndpointId;
            RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-desc-0x%x-dp-index-0x0-dp-number", path, j));
            RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
            pPathDesc2->Descriptor[j].DataPortConfig[0].DataPortNumber = propertyValue;

            RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-desc-0x%x-dp-index-0x0-dp-modes", path, j));
            RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
            pPathDesc2->Descriptor[j].DataPortConfig[0].Modes = propertyValue;

            RETURN_NTSTATUS_IF_FAILED(RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-specialstream-0x%x-desc-0x%x-dp-index-0x0-dp-channel-mask", path, j));
            RETURN_NTSTATUS_IF_FAILED(Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &propertyValue));
            pPathDesc2->Descriptor[j].DataPortConfig[0].ChannelMask = propertyValue;
        }

        // Now save it to circuitCtx
        circuitCtx->SpecialStreamPathDescriptors2[i] = pPathDesc2;
        cleanup2.release();
    }

exit:
    return status;
}

PAGED_CODE_SEG
NTSTATUS CreateStreamBridge(
    _In_ ACX_STREAM_BRIDGE_CONFIG                   StreamCfg,
    _In_ ACXCIRCUIT                                 Circuit,
    _In_ ACXPIN                                     Pin,
    _In_ DSP_PIN_CONTEXT*                           PinCtx,
    _In_ ULONG                                      BridgeDataPortNumber,
    _In_ ULONG                                      BridgeEndpointId,
    _In_opt_ PSDCA_PATH_DESCRIPTORS2                PathDescriptors,
    _In_ BOOL                                       Render
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(EndpointId);
    DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(DataPortNumber);

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    attributes.ParentObject = Pin;

    ACX_OBJECTBAG_CONFIG objBagCfg;
    ACXOBJECTBAG objBag = NULL;
    ACX_OBJECTBAG_CONFIG_INIT(&objBagCfg);
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Circuit;
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagCreate(&attributes, &objBagCfg, &objBag));

    DECLARE_CONST_ACXOBJECTBAG_DRIVER_PROPERTY_NAME(msft, TestUI4);
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUI4(objBag, &TestUI4, _DSP_STREAM_PROPERTY_UI4_VALUE));

    // EndpointId, DataPortNumber, and DPNo included for backwards compatibility.
    // If SdcaPropertyPathDescriptors2 is included in the object bag, these will be ignored.
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUI4(objBag, &EndpointId, BridgeEndpointId));
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUI4(objBag, &DataPortNumber, BridgeDataPortNumber));

    DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(DPNo);
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUI4(objBag, &DPNo, BridgeDataPortNumber));

    if (PathDescriptors && PathDescriptors->Size >= sizeof(SDCA_PATH_DESCRIPTORS2))
    {
        // For uniform aggregated devices and non-aggregated devices, we can save the SdcaPropertyPathDescriptors2
        // now to the stream bridge.
        
        // If the aggregated devices have different configurations (such as a different Channel Mask) the
        // SdcaPropertyPathDescriptors2 should be added to the stream bridge when the pin is connected, since the
        // FunctionInformationId is determined at run time based on the order that the aggregated devices are discovered.

        // Apply the EndpointID to the PathDescriptors structures
        PathDescriptors->EndpointId = BridgeEndpointId;

        // The PathDescriptors->Descriptor[n].DataPortConfig[m].EndpointId value is ignored

        WDFMEMORY pathDescriptorsMemory;
        RETURN_NTSTATUS_IF_FAILED(WdfMemoryCreatePreallocated(WDF_NO_OBJECT_ATTRIBUTES, PathDescriptors, PathDescriptors->Size, &pathDescriptorsMemory));
        auto memory_free = scope_exit([&pathDescriptorsMemory]()
        {
            WdfObjectDelete(pathDescriptorsMemory);
            pathDescriptorsMemory = nullptr;
        });

        // For sample simplicity we always add the path descriptors here.
        // If the EvtPinConnected discovers connected aggregated audio functions it will overwrite this.
        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddBlob(objBag, &SdcaPropertyPathDescriptors2, pathDescriptorsMemory));
    }

    // Save the Object Bag that's being assigned to the stream bridge
    // This will be updated at Pin Connect time if the connected endpoint is aggregated and uses
    // different data ports for each of the aggregated audio functions
    // The AcxObjectBag's lifetime is tied to the Circuit, so the Pin will be able to access it
    // for the Pin's entire lifetime.
    PinCtx->HostStreamObjBag = objBag;

    //
    // Add a stream BRIDGE.
    //
    PCGUID  inModes[] =
    {
        &AUDIO_SIGNALPROCESSINGMODE_RAW,
        &AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
    };

    if (Render) {
        StreamCfg.InModesCount = SIZEOF_ARRAY(inModes);
        StreamCfg.InModes = inModes;
    }

    // Do not specify InModes for capture - this will prevent the ACX framework from adding created streams to this stream
    // bridge automatically. We want to add the stream bridges manually since we don't want KWS streams added.
    StreamCfg.OutMode = &AUDIO_SIGNALPROCESSINGMODE_RAW;
    StreamCfg.OutStreamVarArguments = objBag;

    // Uncomment this line to reverse the change-state sequence notifications. 
    //streamCfg.Flags       |= AcxStreamBridgeInvertChangeStateSequence;

    ACXSTREAMBRIDGE streamBridge = NULL;
    RETURN_NTSTATUS_IF_FAILED(AcxStreamBridgeCreate(Circuit, &attributes, &StreamCfg, &streamBridge));

    if (!Render) {
        PinCtx->HostStreamBridge = streamBridge;
    }

    RETURN_NTSTATUS_IF_FAILED(AcxPinAddStreamBridges(Pin, &streamBridge, 1));

    return status;
}

PAGED_CODE_SEG
NTSTATUS ConnectCaptureCircuitElements(
    _In_ ULONG                                   ElementCount,
    _In_reads_(ElementCount) ACXELEMENT*         Elements,
    _In_ ACXCIRCUIT                              Circuit
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    // connection between each element, plus the connection to the circuit,
    // and an extra connection for the kws pin.
    const int numElements = 3;
    const int numConnections = numElements + 2;

    //
    // Explicitly connect the circuit/elements. Note that driver doens't 
    // need to perform this step when circuit/elements are connected in the 
    // same order as they were added to the circuit. By default ACX connects
    // the elements starting from the sink circuit pin and ending with the 
    // source circuit pin for devices.
    //
    // circuit.pin[default_sink]    -> 1st element.pin[default_in]
    // 1st element.pin[default_out] -> 2nd element.pin[default_in]
    // 2nd element.pin[default_out] -> circuit.pin[default_source]
    //

    ACX_CONNECTION connections[numConnections];
    ACX_CONNECTION_INIT(&connections[0], Circuit, Elements[0]);

    ACX_CONNECTION_INIT(&connections[1], Elements[0], Elements[ElementCount-2]);
    ACX_CONNECTION_INIT(&connections[2], Elements[ElementCount-2], Elements[ElementCount-1]);
    ACX_CONNECTION_INIT(&connections[3], Elements[ElementCount-1], Circuit);
    ACX_CONNECTION_INIT(&connections[4], Elements[ElementCount-1], Circuit);
    connections[4].ToPin.Id = 1;

    //
    // Add the connections linking circuit to elements.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddConnections(Circuit, connections, SIZEOF_ARRAY(connections)));

    return status;
}

PAGED_CODE_SEG
NTSTATUS ConnectRenderCircuitElements(
    _In_ ACXAUDIOENGINE                          AudioEngineElement,
    _In_ ACXCIRCUIT                              Circuit
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Explicitly connect the circuit/elements. Note that driver doesn't 
    // need to perform this step when circuit/elements are connected in the 
    // same order as they were added to the circuit. By default ACX connects
    // the elements starting from the sink circuit pin and ending with the 
    // source circuit pin for both render and capture devices.
    //
    //                           Circuit layout
    //              -----------------------------------------
    //              |                                       |
    //              |        --------------------           |
    //     Host -0->|-----1->|                  |-0-------->|-3-> Bridge Pin
    //              |        |  Audio Engine    |           |
    //  Offload -1->|-----2->|      Node        |-3--|      |
    //              |        |------------------|    |      |
    //              |                                |      |
    // Loopback <-2-|<------------------------------ |      |
    //              |                                       |
    //              |                                       |
    //              |---------------------------------------|
    //

    ACX_CONNECTION connections[4];

    ACX_CONNECTION_INIT(&connections[0], Circuit, AudioEngineElement);
    connections[0].FromPin.Id = DspPinTypeHost;
    connections[0].ToPin.Id = 1;

    ACX_CONNECTION_INIT(&connections[1], Circuit, AudioEngineElement);
    connections[1].FromPin.Id = DspPinTypeOffload;
    connections[1].ToPin.Id = 2;

    ACX_CONNECTION_INIT(&connections[2], AudioEngineElement, Circuit);
    connections[2].ToPin.Id = DspPinTypeLoopback;
    connections[2].FromPin.Id = 3;

    ACX_CONNECTION_INIT(&connections[3], AudioEngineElement, Circuit);
    connections[3].ToPin.Id = DspPinTypeBridge;
    connections[3].FromPin.Id = 0;

    //
    // Add the connections linking circuit to elements.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddConnections(Circuit, connections, SIZEOF_ARRAY(connections)));

    return status;

}

PAGED_CODE_SEG
NTSTATUS CreateAudioEngine(
    _In_ ACXCIRCUIT                             Circuit,
    _In_reads_(DspPinType_Count) ACXPIN*        Pins,
    _Out_ ACXAUDIOENGINE*                       AudioEngineElement
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    /////////////////////////////////////////////////////////
    //
    // Create two elements to handle volume and mute for the audioengine
    // element

    // Mute
    ACX_MUTE_CALLBACKS muteCallbacks;
    ACX_MUTE_CALLBACKS_INIT(&muteCallbacks);
    muteCallbacks.EvtAcxMuteAssignState = DspR_EvtMuteAssignState;
    muteCallbacks.EvtAcxMuteRetrieveState = DspR_EvtMuteRetrieveState;

    ACX_MUTE_CONFIG muteCfg;
    ACX_MUTE_CONFIG_INIT(&muteCfg);
    muteCfg.ChannelsCount = MAX_CHANNELS;
    muteCfg.Name = &KSAUDFNAME_WAVE_MUTE;
    muteCfg.Callbacks = &muteCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_MUTE_ELEMENT_CONTEXT);
    attributes.ParentObject = Circuit;

    ACXMUTE muteElement;
    RETURN_NTSTATUS_IF_FAILED(AcxMuteCreate(Circuit, &attributes, &muteCfg, &muteElement));

    // Volume
    ACX_VOLUME_CALLBACKS volumeCallbacks;
    ACX_VOLUME_CALLBACKS_INIT(&volumeCallbacks);
    volumeCallbacks.EvtAcxRampedVolumeAssignLevel = DspR_EvtRampedVolumeAssignLevel;
    volumeCallbacks.EvtAcxVolumeRetrieveLevel = DspR_EvtVolumeRetrieveLevel;

    ACX_VOLUME_CONFIG volumeCfg;
    ACX_VOLUME_CONFIG_INIT(&volumeCfg);
    volumeCfg.ChannelsCount = MAX_CHANNELS;
    volumeCfg.Minimum = VOLUME_LEVEL_MINIMUM;
    volumeCfg.Maximum = VOLUME_LEVEL_MAXIMUM;
    volumeCfg.SteppingDelta = VOLUME_STEPPING;
    volumeCfg.Name = &KSAUDFNAME_VOLUME_CONTROL;
    volumeCfg.Callbacks = &volumeCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_VOLUME_ELEMENT_CONTEXT);
    attributes.ParentObject = Circuit;

    ACXVOLUME volumeElement;
    RETURN_NTSTATUS_IF_FAILED(AcxVolumeCreate(Circuit, &attributes, &volumeCfg, &volumeElement));

    //
    // Create peakmeter element for Audio engine
    //
    ACX_PEAKMETER_CALLBACKS peakmeterCallbacks;
    ACX_PEAKMETER_CALLBACKS_INIT(&peakmeterCallbacks);
    peakmeterCallbacks.EvtAcxPeakMeterRetrieveLevel = DspR_EvtPeakMeterRetrieveLevelCallback;

    ACX_PEAKMETER_CONFIG peakmeterCfg;
    ACX_PEAKMETER_CONFIG_INIT(&peakmeterCfg);
    peakmeterCfg.ChannelsCount = MAX_CHANNELS;
    peakmeterCfg.Minimum = PEAKMETER_MINIMUM;
    peakmeterCfg.Maximum = PEAKMETER_MAXIMUM;
    peakmeterCfg.SteppingDelta = PEAKMETER_STEPPING_DELTA;
    peakmeterCfg.Callbacks = &peakmeterCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PEAKMETER_ELEMENT_CONTEXT);
    attributes.ParentObject = Circuit;

    ACXPEAKMETER peakmeterElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPeakMeterCreate(Circuit, &attributes, &peakmeterCfg, &peakmeterElement));
    ASSERT(peakmeterElement != NULL);

    PDSP_PEAKMETER_ELEMENT_CONTEXT peakmeterCtx;
    peakmeterCtx = GetDspPeakMeterElementContext(peakmeterElement);
    ASSERT(peakmeterCtx);
    peakmeterCtx->peakMeter = GetDspCircuitContext(Circuit)->peakMeter;

    GetDspCircuitContext(Circuit)->PeakMeterElement = peakmeterElement;

    //
    // Create Audio Engine
    //
    ACX_AUDIOENGINE_CALLBACKS audioEngineCallbacks;
    ACX_AUDIOENGINE_CALLBACKS_INIT(&audioEngineCallbacks);
    audioEngineCallbacks.EvtAcxAudioEngineRetrieveBufferSizeLimits = DspR_EvtAcxAudioEngineRetrieveBufferSizeLimits;
    audioEngineCallbacks.EvtAcxAudioEngineAssignEffectsState = DspR_EvtAcxAudioEngineAssignEffectsState;
    audioEngineCallbacks.EvtAcxAudioEngineRetrieveEffectsState = DspR_EvtAcxAudioEngineRetrieveEffectsState;
    audioEngineCallbacks.EvtAcxAudioEngineRetrieveEngineMixFormat = DspR_EvtAcxAudioEngineRetrieveEngineMixFormat;
    audioEngineCallbacks.EvtAcxAudioEngineAssignEngineDeviceFormat = DspR_EvtAcxAudioEngineAssignEngineDeviceFormat;

    ACX_AUDIOENGINE_CONFIG audioEngineCfg;
    ACX_AUDIOENGINE_CONFIG_INIT(&audioEngineCfg);
    audioEngineCfg.HostPin = Pins[DspPinTypeHost];
    audioEngineCfg.OffloadPin = Pins[DspPinTypeOffload];
    audioEngineCfg.LoopbackPin = Pins[DspPinTypeLoopback];
    audioEngineCfg.VolumeElement = volumeElement;
    audioEngineCfg.MuteElement = muteElement;
    audioEngineCfg.PeakMeterElement = peakmeterElement;
    audioEngineCfg.Callbacks = &audioEngineCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_ENGINE_CONTEXT);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioEngineCreate(Circuit, &attributes, &audioEngineCfg, AudioEngineElement));

    return status;
}

PAGED_CODE_SEG
NTSTATUS SendProperty(
    _In_        WDFOBJECT               AcxTarget,
    _Inout_     PACX_REQUEST_PARAMETERS PropertyParameters,
    _Out_opt_   PULONG_PTR              Information
)
{
    PAGED_CODE();

    if (Information)
    {
        *Information = 0;
    }

    //
    // First step: Determine the WDFIOTARGET to which the property request will be sent
    //
    WDFIOTARGET ioTarget = nullptr;
    if (PropertyParameters->Parameters.Property.ItemType == AcxItemTypePin)
    {
        ioTarget = AcxTargetPinGetWdfIoTarget((ACXTARGETPIN)AcxTarget);
    }
    else if (PropertyParameters->Parameters.Property.ItemType == AcxItemTypeElement)
    {
        ioTarget = AcxTargetElementGetWdfIoTarget((ACXTARGETELEMENT)AcxTarget);
    }
    else if (PropertyParameters->Parameters.Property.ItemType == AcxItemTypeCircuit)
    {
        ioTarget = AcxTargetCircuitGetWdfIoTarget((ACXTARGETCIRCUIT)AcxTarget);
    }
    else
    {
        RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    //
    // Create the request
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = AcxTarget;

    WDFREQUEST request;
    RETURN_NTSTATUS_IF_FAILED(WdfRequestCreate(&attributes, ioTarget, &request));
    auto request_free = scope_exit([&request]()
    {
        WdfObjectDelete(request);
        request = nullptr;
    });

    //
    // ACX framework will format the request properly depending on the type of the target
    //
    if (PropertyParameters->Parameters.Property.ItemType == AcxItemTypePin)
    {
        RETURN_NTSTATUS_IF_FAILED(AcxTargetPinFormatRequestForProperty((ACXTARGETPIN)AcxTarget, request, PropertyParameters));
    }
    else if (PropertyParameters->Parameters.Property.ItemType == AcxItemTypeElement)
    {
        RETURN_NTSTATUS_IF_FAILED(AcxTargetElementFormatRequestForProperty((ACXTARGETELEMENT)AcxTarget, request, PropertyParameters));
    }
    else if (PropertyParameters->Parameters.Property.ItemType == AcxItemTypeCircuit)
    {
        RETURN_NTSTATUS_IF_FAILED(AcxTargetCircuitFormatRequestForProperty((ACXTARGETCIRCUIT)AcxTarget, request, PropertyParameters));
    }

    //
    // Send the request synchronously, with a timeout
    //
    WDF_REQUEST_SEND_OPTIONS sendOptions;
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_SEC(REQUEST_TIMEOUT_SECONDS));

    if (!WdfRequestSend(request, ioTarget, &sendOptions))
    {
        //
        // The framework failed to send the request.
        //
        RETURN_NTSTATUS_IF_FAILED(WdfRequestGetStatus(request));
    }

    //
    // The request was successfully delivered and handled. The status will be based on the target's handling
    //
    if (Information)
    {
        *Information = WdfRequestGetInformation(request);
    }

    return WdfRequestGetStatus(request);
}

// Nonpaged, since this will be called in power up situations
#pragma code_seg()
VOID CircuitRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
    )
/*++

Routine Description:

    This Circuit Request Preprocess routine will forward any Volume
    or Mute requests to the appropriate downstream circuit, if we've
    discovered a downstream circuit that handles Volume and Mute

--*/
{
    NTSTATUS                    status = STATUS_NOT_SUPPORTED;
    ACX_REQUEST_PARAMETERS      params;
    ACX_REQUEST_PARAMETERS      targetParams;
    PDSP_CIRCUIT_CONTEXT        circuitCtx;
    ACXELEMENT                  element;
    ULONG_PTR                   information = 0;
    ACXTARGETELEMENT            targetElement = nullptr;
    GUID                        propertySet;
    ULONG                       propertyId;
    BOOLEAN                     isMute = FALSE;
    BOOLEAN                     isVolume = FALSE;

    // Preprocess will be called very frequently. Don't trace enter/exit.
    //DrvLogEnter(g_SDCAVDspLog);

    UNREFERENCED_PARAMETER(DriverContext);

    ASSERT(Object != NULL);
    ASSERT(DriverContext);
    ASSERT(Request);

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    propertySet = params.Parameters.Property.Set;
    propertyId = params.Parameters.Property.Id;
    circuitCtx = GetDspCircuitContext(Object);

    if (circuitCtx == nullptr ||
        params.Parameters.Property.ItemType != AcxItemTypeElement)
    {
        // We only handle requests for our render circuit (which must have our context)
        // We only forward element requests to the child paths
        (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
        return;
    }

    if (IsEqualGUID(propertySet, KSPROPSETID_Audio) && propertyId == KSPROPERTY_AUDIO_VOLUMELEVEL)
    {
        isVolume = TRUE;
    }
    else if (IsEqualGUID(propertySet, KSPROPSETID_Audio) && propertyId == KSPROPERTY_AUDIO_MUTE)
    {
        isMute = TRUE;
    }
    // Do not forward KSPROPERTY_AUDIOENGINE_VOLUMELEVEL - that is only valid for a stream property.

    if (!isVolume && !isMute)
    {
        // Only handle Volume and Mute requests
        (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
        return;
    }

    element = AcxCircuitGetElementById((ACXCIRCUIT)Object, params.Parameters.Property.ItemId);
    if (!element)
    {
        // We only handle requests for the volume or mute elements, and this isn't an element
        (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
        return;
    }

    if (isVolume)
    {
        targetElement = circuitCtx->TargetVolumeHandler;
    }
    else if (isMute)
    {
        targetElement = circuitCtx->TargetMuteHandler;
    }

    if (targetElement == nullptr)
    {
        // We only handle requests for the volume or mute elements if we have a target.
        (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
        return;
    }

    if (isVolume && (GetDspVolumeElementContext(element) == nullptr && GetDspEngineContext(element) == nullptr))
    {
        // Volume request that isn't for our volume or audioengine element?
        (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
        return;
    }

    if (isMute && (GetDspMuteElementContext(element) == nullptr && GetDspEngineContext(element) == nullptr))
    {
        // Mute request that isn't for our mute or audioengine element?
        (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
        return;
    }

    propertySet = params.Parameters.Property.Set;
    propertyId = params.Parameters.Property.Id;

    ACX_REQUEST_PARAMETERS_INIT_PROPERTY(&targetParams,
                                         propertySet,
                                         propertyId,
                                         params.Parameters.Property.Verb,
                                         params.Parameters.Property.ItemType,
                                         AcxTargetElementGetId(targetElement),
                                         params.Parameters.Property.Control,
                                         params.Parameters.Property.ControlCb,
                                         params.Parameters.Property.Value,
                                         params.Parameters.Property.ValueCb);

    status = SendProperty(targetElement, &targetParams, &information);

    WdfRequestCompleteWithInformation(Request, status, information);
}

PAGED_CODE_SEG
NTSTATUS CreateTargetCircuit(
    _In_    ACXCIRCUIT                  Circuit,
    _In_    PKSPIN_PHYSICALCONNECTION   Connection,
    _In_    ULONG                       ConnectionSize,
    _Out_   ACXTARGETCIRCUIT *          TargetCircuit
)
{
    PAGED_CODE();

    // We have the physical connection. Create a target circuit for it.
    size_t symbolicLinkSize;
    // Size of the string is no more than the total size of the value returned, less the size of the physicalconnection struct,
    // plus the first character of the link (which is included in the physicalconnection struct)
    symbolicLinkSize = ConnectionSize - sizeof(KSPIN_PHYSICALCONNECTION) + sizeof(WCHAR);
    if (symbolicLinkSize > USHORT_MAX)
    {
        // Symbolic Link has to fit in UNICODE_STRING which uses USHORT to hold Length/MaximumLength
        RETURN_NTSTATUS_MSG(STATUS_UNSUCCESSFUL, L"Physical connection too large for unicode_string %lld", symbolicLinkSize);
    }

    UNICODE_STRING symbolicLink{ 0 };
    symbolicLink.MaximumLength = (USHORT)symbolicLinkSize;
    symbolicLink.Buffer = Connection->SymbolicLinkName;
    // preload the length
    (void)RtlStringCbLengthW(symbolicLink.Buffer, symbolicLink.MaximumLength, &symbolicLinkSize);
    symbolicLink.Length = (USHORT)symbolicLinkSize;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Circuit;

    WDFSTRING link;
    RETURN_NTSTATUS_IF_FAILED(WdfStringCreate(&symbolicLink, &attributes, &link));
    auto link_free = scope_exit([&link]()
    {
        if (link)
        {
            WdfObjectDelete(link);
            link = nullptr;
        }
    });

    ACX_TARGET_CIRCUIT_CONFIG targetCktCfg;
    ACX_TARGET_CIRCUIT_CONFIG_INIT(&targetCktCfg);
    targetCktCfg.SymbolicLinkName = link;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxTargetCircuitCreate(AcxCircuitGetWdfDevice(Circuit), &attributes, &targetCktCfg, TargetCircuit));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS FindDownstreamVolumeMute(
    _In_    ACXCIRCUIT          Circuit,
    _In_    ACXTARGETCIRCUIT    TargetCircuit
)
{
    NTSTATUS status;
    PDSP_CIRCUIT_CONTEXT circuitCtx;
    ACX_REQUEST_PARAMETERS  params;

    PAGED_CODE();

    circuitCtx = GetDspCircuitContext(Circuit);

    //
    // Note on behavior: This search algorithm will select the last Volume and Mute elements that are both
    // present in the same circuit in the Endpoint Path.
    // This logic could be updated to select the last Volume and Mute elements, or the first or last
    // Volume or the first or last Mute element.
    //

    //
    // First look through target's pins to determine if there's another circuit downstream.
    // If there is, we'll look at that circuit for volume/mute.
    //
    for (ULONG pinIndex = 0; pinIndex < AcxTargetCircuitGetPinsCount(TargetCircuit); ++pinIndex)
    {
        ACXTARGETPIN targetPin = AcxTargetCircuitGetTargetPin(TargetCircuit, pinIndex);
        ULONG targetPinFlow = 0;
        ACX_REQUEST_PARAMETERS_INIT_PROPERTY(&params,
                                             KSPROPSETID_Pin,
                                             KSPROPERTY_PIN_DATAFLOW,
                                             AcxPropertyVerbGet,
                                             AcxItemTypePin,
                                             AcxTargetPinGetId(targetPin),
                                             nullptr, 0,
                                             &targetPinFlow,
                                             sizeof(targetPinFlow));

        RETURN_NTSTATUS_IF_FAILED(SendProperty(targetPin, &params, nullptr));

        //
        // Searching for the downstream pins. For Render, these are the dataflow out pins
        //
        if (circuitCtx->IsRenderCircuit && targetPinFlow != KSPIN_DATAFLOW_OUT)
        {
            continue;
        }
        else if (!circuitCtx->IsRenderCircuit && targetPinFlow != KSPIN_DATAFLOW_IN)
        {
            continue;
        }

        // Get the target pin's physical connection. We'll do this twice: first to get size and allocate, second to get the connection
        PKSPIN_PHYSICALCONNECTION pinConnection = nullptr;
        auto connection_free = scope_exit([&pinConnection]()
        {
            if (pinConnection)
            {
                ExFreePool(pinConnection);
                pinConnection = nullptr;
            }
        });

        ULONG pinConnectionSize = 0;
        ULONG_PTR info = 0;
        for (ULONG i = 0; i < 2; ++i)
        {
            ACX_REQUEST_PARAMETERS_INIT_PROPERTY(&params,
                                                 KSPROPSETID_Pin,
                                                 KSPROPERTY_PIN_PHYSICALCONNECTION,
                                                 AcxPropertyVerbGet,
                                                 AcxItemTypePin,
                                                 AcxTargetPinGetId(targetPin),
                                                 nullptr, 0,
                                                 pinConnection,
                                                 pinConnectionSize);

            status = SendProperty(targetPin, &params, &info);

            if (status == STATUS_BUFFER_OVERFLOW)
            {
                // Pin connection already allocated, so how did this fail?
                RETURN_NTSTATUS_IF_TRUE(pinConnection != nullptr, status);

                pinConnectionSize = (ULONG)info;
                pinConnection = (PKSPIN_PHYSICALCONNECTION)ExAllocatePool2(POOL_FLAG_NON_PAGED, pinConnectionSize, DRIVER_TAG);
                // RETURN_NTSTATUS_IF_NULL_ALLOC causes compile errors
                RETURN_NTSTATUS_IF_TRUE(pinConnection == nullptr, STATUS_INSUFFICIENT_RESOURCES);
            }
            else if (!NT_SUCCESS(status))
            {
                // There are no more connected circuits. Continue with processing this circuit.
                break;
            }
        }

        if (!NT_SUCCESS(status))
        {
            // There are no more connected circuits. Continue handling this circuit.
            break;
        }

        ACXTARGETCIRCUIT nextTargetCircuit;
        RETURN_NTSTATUS_IF_FAILED(CreateTargetCircuit(Circuit, pinConnection, pinConnectionSize, &nextTargetCircuit));
        auto circuit_free = scope_exit([&nextTargetCircuit]()
        {
            if (nextTargetCircuit)
            {
                WdfObjectDelete(nextTargetCircuit);
                nextTargetCircuit = nullptr;
            }
        });

        RETURN_NTSTATUS_IF_FAILED_UNLESS_ALLOWED(FindDownstreamVolumeMute(Circuit, nextTargetCircuit), STATUS_NOT_FOUND);
        if (circuitCtx->TargetVolumeMuteCircuit == nextTargetCircuit)
        {
            circuitCtx->TargetCircuitToDelete = nextTargetCircuit;

            // The nextTargetCircuit is the owner of the volume/mute target elements.
            // We will delete it when the pin is disconnected.
            circuit_free.release();

            // We found volume/mute. Return.
            return STATUS_SUCCESS;
        }

        // There's only one downstream pin on the current targetcircuit, and we just processed it.
        break;
    }

    //
    // Search the target circuit for a volume or mute element.
    // This sample code doesn't support downstream audioengine elements.
    // 
    for (ULONG elementIndex = 0; elementIndex < AcxTargetCircuitGetElementsCount(TargetCircuit); ++elementIndex)
    {
        ACXTARGETELEMENT targetElement = AcxTargetCircuitGetTargetElement(TargetCircuit, elementIndex);
        GUID elementType = AcxTargetElementGetType(targetElement);

        if (IsEqualGUID(elementType, KSNODETYPE_VOLUME) &&
            circuitCtx->TargetVolumeHandler == nullptr)
        {
            // Found Volume
            circuitCtx->TargetVolumeHandler = targetElement;
        }
        if (IsEqualGUID(elementType, KSNODETYPE_MUTE) &&
            circuitCtx->TargetMuteHandler == nullptr)
        {
            // Found Mute
            circuitCtx->TargetMuteHandler = targetElement;
        }
    }

    if (circuitCtx->TargetVolumeHandler && circuitCtx->TargetMuteHandler)
    {
        circuitCtx->TargetVolumeMuteCircuit = TargetCircuit;
        return STATUS_SUCCESS;
    }

    //
    // If we only found one of volume or mute, keep searching for both
    //
    if (circuitCtx->TargetVolumeHandler || circuitCtx->TargetMuteHandler)
    {
        circuitCtx->TargetMuteHandler = circuitCtx->TargetVolumeHandler = nullptr;
    }

    return STATUS_NOT_FOUND;
}

PAGED_CODE_SEG
NTSTATUS
ReplicateFormatsForAudioEngine(
    _In_ ACXAUDIOENGINE     AudioEngine,
    _In_ ACXTARGETCIRCUIT   TargetCircuit,
    _In_ ULONG              TargetPinId
)
{
    PAGED_CODE();

    ACXTARGETPIN targetPin;
    targetPin = AcxTargetCircuitGetTargetPin(TargetCircuit, TargetPinId);
    if (!targetPin)
    {
        RETURN_NTSTATUS(STATUS_UNSUCCESSFUL);
    }

    ACXDATAFORMATLIST targetFormatList;
    // We expect at least Raw format in SDCA downstream circuits
    RETURN_NTSTATUS_IF_FAILED(AcxTargetPinRetrieveModeDataFormatList(targetPin, &AUDIO_SIGNALPROCESSINGMODE_RAW, &targetFormatList));

    ACXDATAFORMATLIST localFormatList = AcxAudioEngineGetDeviceFormatList(AudioEngine);

    RETURN_NTSTATUS_IF_FAILED(SdcaVad_ClearDataFormatList(localFormatList));

    ULONG formatCount = 0;
    RETURN_NTSTATUS_IF_FAILED(SdcaVad_CopyFormats(targetFormatList, localFormatList, &formatCount));

    if (formatCount == 0)
    {
        RETURN_NTSTATUS(STATUS_NO_MATCH);
    }

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
ReplicateFormatsForPin(
    _In_ ACXPIN             Pin,
    _In_ ACXTARGETCIRCUIT   TargetCircuit,
    _In_ ULONG              TargetPinId
)
{
    PAGED_CODE();

    ACXTARGETPIN targetPin;
    targetPin = AcxTargetCircuitGetTargetPin(TargetCircuit, TargetPinId);
    if (!targetPin)
    {
        RETURN_NTSTATUS(STATUS_UNSUCCESSFUL);
    }

    // Don't delete the target pin - it will be cleaned up when the target circuit is cleaned up by ACX

    GUID targetModes[] =
    {
        AUDIO_SIGNALPROCESSINGMODE_RAW,
        AUDIO_SIGNALPROCESSINGMODE_DEFAULT,
        AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS,
        AUDIO_SIGNALPROCESSINGMODE_SPEECH
    };

    ULONG totalFormats = 0;

    for (ULONG modeIdx = 0; modeIdx < ARRAYSIZE(targetModes); ++modeIdx)
    {
        ACXDATAFORMATLIST targetFormatList;
        ACXDATAFORMATLIST localFormatList = nullptr;

        NTSTATUS status = AcxTargetPinRetrieveModeDataFormatList(targetPin, targetModes + modeIdx, &targetFormatList);
        if (!NT_SUCCESS(status))
        {
            // If the downstream pin doesn't support any formats for this mode, make sure we clear out our pin's
            // formats for this mode as well.
            if (modeIdx == 0)
            {
                localFormatList = AcxPinGetRawDataFormatList(Pin);
            }
            else
            {
                // Ignore the status
                AcxPinRetrieveModeDataFormatList(Pin, targetModes + modeIdx, &localFormatList);
            }
            if (localFormatList)
            {
                RETURN_NTSTATUS_IF_FAILED(SdcaVad_ClearDataFormatList(localFormatList));
            }
            continue;
        }

        RETURN_NTSTATUS_IF_FAILED(SdcaVad_RetrieveOrCreateDataFormatList(Pin, targetModes + modeIdx, &localFormatList));

        RETURN_NTSTATUS_IF_FAILED(SdcaVad_ClearDataFormatList(localFormatList));

        ULONG formatCount = 0;
        RETURN_NTSTATUS_IF_FAILED(SdcaVad_CopyFormats(targetFormatList, localFormatList, &formatCount));

        totalFormats += formatCount;
    }

    if (totalFormats == 0)
    {
        return STATUS_NO_MATCH;
    }

    return STATUS_SUCCESS;
}


