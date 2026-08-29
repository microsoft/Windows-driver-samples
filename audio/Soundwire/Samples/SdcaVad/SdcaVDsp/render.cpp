/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    render.cpp

Abstract:

    Render factory and circuit

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
#include "offloadStreamEngine.h"
#include "SimPeakMeter.h"
#include "CircuitHelper.h"
#include "AcpiReader.h"

#include "TestProperties.h"
#include "AudioFormats.h"

#ifndef __INTELLISENSE__
#include "render.tmh"
#endif

#include "audiomodule.h"

using namespace ACPIREADER;

//
// max # of streams for each pin type.
//
#define DSPR_MAX_INPUT_HOST_STREAMS         2
#define DSPR_MAX_INPUT_OFFLOAD_STREAMS      3
#define DSPR_MAX_OUTPUT_LOOPBACK_STREAMS    1

//
// Factory circuit IDs.
//
#define RENDER_DEVICE_ID_STR L"{4DCB0606-6415-4A36-BDC5-9B1792117DC9}\\Render&CP_%wZ"
DECLARE_CONST_UNICODE_STRING(RenderHardwareId, L"{4DCB0606-6415-4A36-BDC5-9B1792117DC9}\\Render");

DECLARE_CONST_UNICODE_STRING(RenderCompatibleId, ACX_DSP_TEST_COMPATIBLE_ID);
DECLARE_CONST_UNICODE_STRING(RenderContainerId,  ACX_DSP_TEST_CONTAINER_ID);
DECLARE_CONST_UNICODE_STRING(RenderDeviceLocation, L"SDCAVDsp Dynamic Enum Speaker");

PAGED_CODE_SEG
VOID
DspR_EvtPinCInstancesCallback(
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
DspR_EvtPinCTypesCallback(
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
DspR_EvtPinDataFlowCallback(
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
DspR_EvtPinDataRangesCallback(
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
DspR_EvtPinDataIntersectionCallback(
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
DspR_EvtAcxPinSetDataFormat (
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
DSP_SendPropertyTo
(
    _In_ WDFDEVICE Device,
    _In_ ACXTARGETCIRCUIT TargetCircuit,
    _In_ GUID PropertySet,
    _In_ ULONG PropertyId,
    _In_ ACX_PROPERTY_VERB Verb,
    _In_ PVOID Control,
    _In_ ULONG ControlCb,
    _Inout_ PVOID Value,
    _In_ ULONG ValueCb,
    _Out_ ULONG_PTR* Information
)
{
    PAGED_CODE();

    ACX_REQUEST_PARAMETERS requestParams;
    ACX_REQUEST_PARAMETERS_INIT_PROPERTY(
        &requestParams,
        PropertySet,
        PropertyId,
        Verb,
        AcxItemTypeCircuit,
        0,
        Control, ControlCb,
        Value, ValueCb
    );

    WDFREQUEST request;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;
    RETURN_NTSTATUS_IF_FAILED(WdfRequestCreate(&attributes, AcxTargetCircuitGetWdfIoTarget(TargetCircuit), &request));

    auto request_free = scope_exit([&request]() {
        WdfObjectDelete(request);
        });

    RETURN_NTSTATUS_IF_FAILED(AcxTargetCircuitFormatRequestForProperty(TargetCircuit, request, &requestParams));

    WDF_REQUEST_SEND_OPTIONS sendOptions;
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_SEC(5));

    RETURN_NTSTATUS_IF_TRUE(!WdfRequestSend(request, AcxTargetCircuitGetWdfIoTarget(TargetCircuit), &sendOptions), STATUS_INVALID_DEVICE_REQUEST);

    NTSTATUS status = WdfRequestGetStatus(request);
    if (Information)
    {
        *Information = WdfRequestGetInformation(request);
    }
    if (status == STATUS_BUFFER_OVERFLOW && ValueCb == 0)
    {
        // Don't trace this error, it's normal
        return status;
    }

    RETURN_NTSTATUS_IF_FAILED(status);

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
DspR_AssignAggregatedDataPorts(
    _In_    ACXCIRCUIT  Circuit,
    _In_    ACXPIN      Pin
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    const ULONG MAX_EXPECTED_AGGREGATED_DEVICES = 16;
    // There will be one data port in this array for each aggregated device.
    ULONG dataPortPerFunction[MAX_EXPECTED_AGGREGATED_DEVICES];
    ULONG dataPortPerFunctionCount = 0;
    struct DataPortMap
    {
        ULONG FunctionId;
        ULONG DataPortNumber;
    };

    // This is an example of one way the Function ID could be used to determine which data port should be used
    // Note that the order of the Audio Functions is not determistic. We will recalculate the data port array
    // each time our circuit's pin is connected to the aggregator's pin.
    // Note that until the pin connection is made there is no way to determine what order the audio functions
    // will be indexed by.
    //
    // In most or all cases for real-world drivers, this information should be loaded from the ACPI audio composition
    // tables as an array of mappings between Function ID and Data Port. In the case of conflicting Function IDs the
    // streaming driver could also include FunctionManufacturerId when determining which data port to use.
    DataPortMap dataPortMapping[] =
    {
        {0x6798, 0x1}, // Example Function ID of 6798
        {0x5037, 0x3}, // Example Function ID of 5037
    };

    DSP_PIN_CONTEXT* pinCtx = GetDspPinContext(Pin);
    PDSP_CIRCUIT_CONTEXT circuitCtx = GetDspCircuitContext(Circuit);

    PAGED_CODE();

    if (circuitCtx->ConnectedFunctionInformation &&
        circuitCtx->ConnectedFunctionInformation->FunctionCount >= 1)
    {
        // The DataPortNumbers entry is required for aggregated systems that use different data port numbers for each
        // connected audio function.
        // The DataPortNumbers entry can also be used for non-aggregated systems, where the single value will be used
        // instead of DPNo.
        // For aggregated systems that use the same data port number for each connected audio function, DataPortNumbers
        // must still have one entry for each audio function if it is used.
        for (ULONG i = 0; i < circuitCtx->ConnectedFunctionInformation->FunctionCount; ++i)
        {
            // For each of the devices that's being aggregated, we will determine if we have a data port for the device
            // in the mapping. If so, we will assign that data port to the device's index in the array of data ports we
            // will add to the VarArguments for the stream bridge.

            for (ULONG mapIdx = 0; mapIdx < ARRAYSIZE(dataPortMapping); ++mapIdx)
            {
                if (dataPortMapping[mapIdx].FunctionId == circuitCtx->ConnectedFunctionInformation->FunctionInfoList[i].FunctionId)
                {
                    // The Audio Function at index 'i' has the same Function Id as this mapping entry.
                    dataPortPerFunction[i] = dataPortMapping[mapIdx].DataPortNumber;

                    // We want to ensure we have a data port in our map for each audio function
                    ++dataPortPerFunctionCount;
                    break;
                }
            }
        }
    }

    ASSERT((dataPortPerFunctionCount == 0) || (dataPortPerFunctionCount == circuitCtx->ConnectedFunctionInformation->FunctionCount));

    if ((dataPortPerFunctionCount > 0) && (dataPortPerFunctionCount == circuitCtx->ConnectedFunctionInformation->FunctionCount))
    {
        // The SdcaAggregator driver will override DPNo for each aggregated device with the value in that device's index in the
        // DataPortNumbers array.
        DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(DataPortNumbers);
        WDFMEMORY dataPortMemory = nullptr;

        RETURN_NTSTATUS_IF_FAILED(WdfMemoryCreatePreallocated(nullptr, dataPortPerFunction, sizeof(ULONG)* dataPortPerFunctionCount, &dataPortMemory));
        auto dataPortMemory_free = scope_exit([&dataPortMemory]()
        {
            WdfObjectDelete(dataPortMemory);
        });

        // Add the DataPortNumbers to the AcxObjectBag that was assigned to the Stream Bridge during circuit creation.
        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddBlob(pinCtx->HostStreamObjBag, &DataPortNumbers, dataPortMemory));
    }
    else if (dataPortPerFunctionCount > 0)
    {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        DrvLogError(g_SDCAVDspLog, FLAG_INFO, L"Found aggregated data port entry, but not for every audio function, %!STATUS!", status);
    }
    // If dataPortPerFunctionCount is 0, there aren't specific data ports per audio function and SdcaAggregator can leave DPNo as is for
    // each of the different audio functions.

    return status;
}

PAGED_CODE_SEG
NTSTATUS
DspR_AssignAggregatedPathDescriptors(
    _In_    ACXCIRCUIT  Circuit,
    _In_    ACXPIN      Pin
)
{
    WDFMEMORY descriptorsMemory = nullptr;
    PSDCA_PATH_DESCRIPTORS2 descriptorsBuffer = nullptr;

    DSP_PIN_CONTEXT* pinCtx = GetDspPinContext(Pin);
    PDSP_CIRCUIT_CONTEXT circuitCtx = GetDspCircuitContext(Circuit);

    PAGED_CODE();

    if (circuitCtx->AggregatedPathDescriptors == nullptr)
    {
        return STATUS_SUCCESS;
    }

    // Note that in the companion amp scenario some DSP drivers may not include aggregated path descriptor information
    // for the companion amps. In that case, the connected function information count (which includes companions)
    // will be more than the desciptor count.
    if (!circuitCtx->ConnectedFunctionInformation ||
        circuitCtx->ConnectedFunctionInformation->FunctionCount < circuitCtx->AggregatedPathDescriptors->DescriptorCount)
    {
        return STATUS_SUCCESS;
    }

    RETURN_NTSTATUS_IF_FAILED(WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES, NonPagedPoolNx, DRIVER_TAG, circuitCtx->AggregatedPathDescriptors->Size, &descriptorsMemory, (PVOID*)&descriptorsBuffer));
    auto free_memory = scope_exit([&descriptorsMemory]()
    {
        WdfObjectDelete(descriptorsMemory);
    });

    // Copy over entirely; we'll fix up the Function Information Id inplace
    RtlCopyMemory(descriptorsBuffer, circuitCtx->AggregatedPathDescriptors, circuitCtx->AggregatedPathDescriptors->Size);

    ULONG fixedUpDescriptors = 0;
    for (ULONG i = 0; i < circuitCtx->AggregatedPathDescriptors->DescriptorCount; ++i)
    {
        // For each of the aggregated devices, we need look it up by the UniqueID in the list of path descriptors
        // we have.
        for (ULONG connected = 0; connected < circuitCtx->ConnectedFunctionInformation->FunctionCount; ++connected)
        {
            // When saving the aggregated path descriptors, we stored the Function Info Unique ID in the descriptor's FunctionInformationId
            // We use the Function Info Unique ID here to determine the correct FunctionInformationId for the aggregated device.
            // The order of the aggregated devices can change depending on a lot of factors, so we need to use the Unique ID to get the right
            // FunctionInformationId for each device.
            if (circuitCtx->AggregatedPathDescriptors->Descriptor[i].FunctionInformationId == circuitCtx->ConnectedFunctionInformation->FunctionInfoList[connected].UniqueId)
            {
                descriptorsBuffer->Descriptor[i].FunctionInformationId = circuitCtx->ConnectedFunctionInformation->FunctionInfoList[connected].FunctionInformationId;
                ++fixedUpDescriptors;
                break;
            }
        }
    }

    if (fixedUpDescriptors != descriptorsBuffer->DescriptorCount)
    {
        RETURN_NTSTATUS_IF_FAILED(STATUS_DEVICE_CONFIGURATION_ERROR);
    }

    descriptorsBuffer->EndpointId = circuitCtx->EndpointId;

    // Add the DataPortNumbers to the AcxObjectBag that was assigned to the Stream Bridge during circuit creation.
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddBlob(pinCtx->HostStreamObjBag, &SdcaPropertyPathDescriptors2, descriptorsMemory));

    return STATUS_SUCCESS;
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
DspR_EvtPinConnected (
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

    // The bridge pin should support the same formats that are supported by the downstream circuit
    // We could also change the formats supported by the host pin here, but a DSP will typically determine
    // those formats and do appropriate processing.
    ACXPIN bridgePin = AcxCircuitGetPinById(AcxPinGetCircuit(Pin), DspPinTypeBridge);
    NTSTATUS status = ReplicateFormatsForPin(bridgePin, TargetCircuit, TargetPinId);
    if (!NT_SUCCESS(status))
    {
        DrvLogError(g_SDCAVDspLog, FLAG_STREAM, L"Failed to replicate downstream formats to bridge pin, %!STATUS!",
            status);
    }

    ACXAUDIOENGINE audioEngine = GetDspCircuitContext(AcxPinGetCircuit(Pin))->AudioEngineElement;
    status = ReplicateFormatsForAudioEngine(audioEngine, TargetCircuit, TargetPinId);
    if (!NT_SUCCESS(status))
    {
        DrvLogError(g_SDCAVDspLog, FLAG_STREAM, L"Failed to replicate downstream formats to audio engine, %!STATUS!",
            status);
    }

    // The ACX framework will maintain the TargetCircuit until after it's called EvtPinDisconnected

    ACXCIRCUIT circuit = AcxPinGetCircuit(Pin);
    status = FindDownstreamVolumeMute(circuit, TargetCircuit);
    if (!NT_SUCCESS(status))
    {
        DrvLogWarning(g_SDCAVDspLog, FLAG_INIT, L"Unable to find downstream volume/mute elements. Volume and Mute forwarding will be disabled. %!STATUS!", status);
    }

    PDSP_CIRCUIT_CONTEXT circuitCtx = GetDspCircuitContext(circuit);

    // Preallocate enough room to hold information for maximum expected aggregated devices.
    struct AggregationDevices
    {
        SDCA_AGGREGATION_DEVICES Devices;
        SDCA_AGGREGATION_DEVICE  DeviceExtra[MAX_AGGREGATED_DEVICES-1];
    };
    AggregationDevices aggDevices{ 0 };
    aggDevices.Devices.Size = sizeof(aggDevices);
    // The DSP driver should know from the ACPI composition tables whether
    // this circuit is connected to an aggregated endpoint. However, in the
    // meantime, we will just ask the target circuit.
    status = DSP_SendPropertyTo(
        AcxCircuitGetWdfDevice(circuit),
        TargetCircuit,
        KSPROPERTYSETID_SdcaAgg,
        KSPROPERTY_SDCAAGG_AGGREGATED_DEVICES,
        AcxPropertyVerbGet,
        nullptr, 0,
        &aggDevices,
        sizeof(aggDevices),
        nullptr
    );

    if (NT_SUCCESS(status))
    {
        circuitCtx->Aggregated = TRUE;
        circuitCtx->AggregatedDeviceCount = aggDevices.Devices.FunctionCount;
        for (ULONG i = 0; i < aggDevices.Devices.FunctionCount; ++i)
        {
            RtlCopyMemory(&circuitCtx->AggregatedDevices[i], &aggDevices.Devices.FunctionIds[i], sizeof(SDCA_AGGREGATION_DEVICE));
        }
    }

    // Delete previous ConnectedFunctionInformation if any is already allocated
    if (circuitCtx->ConnectedFunctionInformation)
    {
        ExFreePool(circuitCtx->ConnectedFunctionInformation);
        circuitCtx->ConnectedFunctionInformation = nullptr;
    }

    ULONG_PTR requiredBufferSize;

    // retrieve the function information for this device. We'll use this information if the device
    // has special stream capabilities.
    // We'll also use this information if this is an aggregated device that has different Data Port requirements for the audio functions.
    status = DSP_SendPropertyTo(
        AcxCircuitGetWdfDevice(circuit),
        TargetCircuit,
        KSPROPERTYSETID_Sdca,
        KSPROPERTY_SDCA_FUNCTION_INFORMATION,
        AcxPropertyVerbGet,
        nullptr, 0,
        nullptr, 0,
        &requiredBufferSize);

    if (status == STATUS_BUFFER_OVERFLOW &&
        requiredBufferSize >= sizeof(SDCA_FUNCTION_INFORMATION_LIST))
    {
        circuitCtx->ConnectedFunctionInformation = (PSDCA_FUNCTION_INFORMATION_LIST)ExAllocatePool2(POOL_FLAG_NON_PAGED, requiredBufferSize, DRIVER_TAG);
        if (!circuitCtx->ConnectedFunctionInformation)
        {
            return;
        }

        status = DSP_SendPropertyTo(
            AcxCircuitGetWdfDevice(circuit),
            TargetCircuit,
            KSPROPERTYSETID_Sdca,
            KSPROPERTY_SDCA_FUNCTION_INFORMATION,
            AcxPropertyVerbGet,
            nullptr, 0,
            circuitCtx->ConnectedFunctionInformation, (ULONG)requiredBufferSize,
            nullptr);
    }

    if (circuitCtx->ConnectedFunctionInformation && circuitCtx->ConnectedFunctionInformation->FunctionCount > 1)
    {
        // Since FunctionCount is > 1, this is an aggregated system. As such we should update the stream bridge's VarArguments Bag
        // to include a list of data ports based on the devices being aggregated.
        // This is necessary if the aggregated audio functions are not uniform and use different data ports for their inputs.
        status = DspR_AssignAggregatedDataPorts(circuit, Pin);
        if (!NT_SUCCESS(status))
        {
            DrvLogWarning(g_SDCAVDspLog, FLAG_INIT, L"Unable to assign data ports for aggregated connection. %!STATUS!", status);
        }

        // To specify channel mask or more information, the path descriptors structure needs to be used
        status = DspR_AssignAggregatedPathDescriptors(circuit, Pin);
        if (!NT_SUCCESS(status))
        {
            DrvLogWarning(g_SDCAVDspLog, FLAG_INIT, L"Unable to assign path descriptors for aggregated connection. %!STATUS!", status);
        }
    }

    // retrieve the special stream capabilities for the downstream device
    status = DSP_SendPropertyTo(
        AcxCircuitGetWdfDevice(circuit),
        TargetCircuit,
        KSPROPERTYSETID_Sdca,
        KSPROPERTY_SDCA_FUNCTION_CAPABILITY,
        AcxPropertyVerbGet,
        NULL, 0,
        &circuitCtx->SpecialStreamAvailablePaths, sizeof(SDCA_PATH),
        nullptr);

    if (NT_SUCCESS(status))
    {
        // we have path information for a target circuit which supports
        // special paths. collect/refresh our cached information.
        if (circuitCtx->SpecialStreamTargetCircuit)
        {
            // ACX will not call EvtPinConnected more than once without
            // calling EvtPinDisconnected between, so SpecialStreamTargetCircuit
            // should be NULL here.
            ASSERT(FALSE);
        }

        // Since we'll clean this up in EvtPinDisconnected we do not
        // need to perform WdfObjectReference on the TargetCircuit here.
        circuitCtx->SpecialStreamTargetCircuit = TargetCircuit;

        for(ULONG i = (UINT) SpecialStreamTypeUltrasoundRender; i < (UINT) SpecialStreamType_Count; i++)
        {
            if (circuitCtx->SpecialStreamPathDescriptors[i])
            {
                ExFreePool(circuitCtx->SpecialStreamPathDescriptors[i]);
                circuitCtx->SpecialStreamPathDescriptors[i] = nullptr;
            }
        }
    }

    // go through the capabilities and query each that is supported for the descriptors
    for(ULONG i = (UINT) SpecialStreamTypeUltrasoundRender; i < (UINT) SpecialStreamType_Count; i++)
    {
        SDCA_PATH currentPath = SdcaPathFromSpecialStreamType((SDCA_SPECIALSTREAM_TYPE) i);

        if ((circuitCtx->SpecialStreamAvailablePaths & currentPath) != 0)
        {
            // The descriptor is a variable length structure, so
            // we need to first determine the size required
            status = DSP_SendPropertyTo(
                AcxCircuitGetWdfDevice(circuit),
                TargetCircuit,
                KSPROPERTYSETID_Sdca,
                KSPROPERTY_SDCA_PATH_DESCRIPTORS,
                AcxPropertyVerbGet,
                &currentPath, sizeof(SDCA_PATH),
                nullptr, 0,
                &requiredBufferSize);

            // buffer overflow indicates that the descriptorSize has been filled in with
            // the required buffer size. It should be at least a SDCA_PATH_DESCRIPTORS worth
            // of data, more depending on formats supported.
            if (status == STATUS_BUFFER_OVERFLOW &&
                requiredBufferSize >= sizeof(SDCA_PATH_DESCRIPTORS))
            {
                // now that we know the size, allocate and retrieve it.
                circuitCtx->SpecialStreamPathDescriptors[i] = (PSDCA_PATH_DESCRIPTORS) ExAllocatePool2(POOL_FLAG_NON_PAGED, requiredBufferSize, DRIVER_TAG);
                if (circuitCtx->SpecialStreamPathDescriptors[i])
                {
                    status = DSP_SendPropertyTo(
                        AcxCircuitGetWdfDevice(circuit),
                        TargetCircuit,
                        KSPROPERTYSETID_Sdca,
                        KSPROPERTY_SDCA_PATH_DESCRIPTORS,
                        AcxPropertyVerbGet,
                        &currentPath, sizeof(SDCA_PATH),
                        circuitCtx->SpecialStreamPathDescriptors[i], (ULONG) requiredBufferSize,
                        nullptr);
                }
            }
        }
    }
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
DspR_EvtPinDisconnected (
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

    // We cannot use the TargetCircuit after returning from EvtPinDisconnected
    if (pinCtx->TargetCircuit)
    {
        pinCtx->TargetCircuit = NULL;
        pinCtx->TargetPinId = (ULONG)(-1);
    }

    ACXCIRCUIT circuit = AcxPinGetCircuit(Pin);
    PDSP_CIRCUIT_CONTEXT circuitCtx = GetDspCircuitContext(circuit);
    if (circuitCtx->TargetVolumeMuteCircuit)
    {
        circuitCtx->TargetMuteHandler = nullptr;
        circuitCtx->TargetVolumeHandler = nullptr;
        circuitCtx->TargetVolumeMuteCircuit = nullptr;
    }
    if (circuitCtx->TargetCircuitToDelete)
    {
        WdfObjectDelete(circuitCtx->TargetCircuitToDelete);
        circuitCtx->TargetCircuitToDelete = nullptr;
    }

    circuitCtx->SpecialStreamAvailablePaths = 0;

    for(ULONG i = (UINT) SpecialStreamTypeUltrasoundRender; i < (UINT) SpecialStreamType_Count; i++)
    {
        if (circuitCtx->SpecialStreamPathDescriptors[i])
        {
            ExFreePool(circuitCtx->SpecialStreamPathDescriptors[i]);
            circuitCtx->SpecialStreamPathDescriptors[i] = nullptr;
        }
    }

    if (circuitCtx->SpecialStreamTargetCircuit)
    {
        circuitCtx->SpecialStreamTargetCircuit = nullptr;
    }

    if (circuitCtx->ConnectedFunctionInformation)
    {
        ExFreePool(circuitCtx->ConnectedFunctionInformation);
        circuitCtx->ConnectedFunctionInformation = nullptr;
    }
}

PAGED_CODE_SEG
NTSTATUS
DspR_EvtDevicePrepareHardware(
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
    PDSP_RENDER_DEVICE_CONTEXT      devCtx;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    devCtx = GetRenderDeviceContext(Device);
    ASSERT(devCtx != NULL);

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCA VDSP %p Prepare Hardware, First Time %d", Device, devCtx->FirstTimePrepareHardware);

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
    RETURN_NTSTATUS_IF_FAILED(DspR_SetPowerPolicy(Device));

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
DspR_EvtDeviceReleaseHardware(
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
    NTSTATUS                        status;
    PDSP_RENDER_DEVICE_CONTEXT      devCtx;

    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    devCtx = GetRenderDeviceContext(Device);
    ASSERT(devCtx != NULL);

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCA VDSP %p Release Hardware", Device);

    status = STATUS_SUCCESS;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
DspR_EvtDeviceSelfManagedIoInit(
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
    PDSP_RENDER_DEVICE_CONTEXT    devCtx;

    PAGED_CODE();

    devCtx = GetRenderDeviceContext(Device);
    ASSERT(devCtx != NULL);

    return STATUS_SUCCESS;
}

#pragma code_seg()
VOID
DspR_EvtDeviceContextCleanup(
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
    PDSP_RENDER_DEVICE_CONTEXT      devCtx;

    device = (WDFDEVICE)WdfDevice;
    devCtx = GetRenderDeviceContext(device);
    ASSERT(devCtx != NULL);

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCA VDSP Device Cleanup %p", WdfDevice);
}

#pragma code_seg()
VOID
DspR_EvtCircuitContextCleanup(
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

    if (circuitCtx->peakMeter)
    {
        CSimPeakMeter* peakMeter = (CSimPeakMeter *)circuitCtx->peakMeter;
        delete peakMeter;
        circuitCtx->peakMeter = NULL;
    }

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
        circuitCtx->SpecialStreamTargetCircuit = nullptr;
    }

    if (circuitCtx->ConnectedFunctionInformation)
    {
        ExFreePool(circuitCtx->ConnectedFunctionInformation);
        circuitCtx->ConnectedFunctionInformation = nullptr;
    }

    if (circuitCtx->AggregatedPathDescriptors)
    {
        ExFreePool(circuitCtx->AggregatedPathDescriptors);
        circuitCtx->AggregatedPathDescriptors = nullptr;
    }

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCA VDSP Circuit Cleanup %p", Circuit);
}

#pragma code_seg()
VOID
DspR_EvtPinContextCleanup(
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
        pinCtx->TargetCircuit = NULL;
        pinCtx->TargetPinId = (ULONG)(-1);
    }
}

#pragma code_seg()
VOID
DspR_EvtCircuitRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
)
/*++

Routine Description:

    This function is an example of a preprocess routine.

--*/
{
    CircuitRequestPreprocess(Object, DriverContext, Request);
}

PAGED_CODE_SEG
VOID
DspR_EvtStreamRequestPreprocess(
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
DspR_SetPowerPolicy(
    _In_ WDFDEVICE      Device
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Init the idle policy structure.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeout = 10000; // 10-sec

    RETURN_NTSTATUS_IF_FAILED(WdfDeviceAssignS0IdleSettings(Device, &idleSettings));

    return status;
}

PAGED_CODE_SEG
NTSTATUS DspR_EvtAcxFactoryCircuitCreateCircuitDevice(
    _In_  WDFDEVICE                         Parent,
    _In_  ACXFACTORYCIRCUIT                 Factory,
    _In_  PACX_FACTORY_CIRCUIT_ADD_CIRCUIT  CircuitConfig,
    _Out_ WDFDEVICE *                       Device
)
{
    PAGED_CODE();

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
    RETURN_NTSTATUS_IF_TRUE(NULL == wstrBuffer, STATUS_INSUFFICIENT_RESOURCES);
    auto wstrBuffer_free = scope_exit([&wstrBuffer](){
        delete [] wstrBuffer;
    });

    RtlZeroMemory(wstrBuffer, sizeof(WCHAR) * wstrBufferCch);

    //
    // Create a child audio device for this circuit.
    //
    PWDFDEVICE_INIT devInit = NULL;
    devInit = WdfPdoInitAllocate(Parent);
    RETURN_NTSTATUS_IF_TRUE(NULL == devInit, STATUS_INSUFFICIENT_RESOURCES);
    auto devInit_free = scope_exit([&devInit]() {
        WdfDeviceInitFree(devInit);
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
        GUID                    uniqueId = { 0 };
        UNICODE_STRING          uniqueIdStr = { 0 };
        UNICODE_STRING          pnpDeviceId = { 0 };
        ACX_OBJECTBAG_CONFIG    objBagCfg;

        DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(UniqueID);

        ACX_OBJECTBAG_CONFIG_INIT(&objBagCfg);
        objBagCfg.Handle = CircuitConfig->CompositeProperties;
        objBagCfg.Flags |= AcxObjectBagConfigOpenWithHandle;

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        ACXOBJECTBAG objBag = NULL;

        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagOpen(&attributes, &objBagCfg, &objBag));
        auto objBag_free = scope_exit([&objBag]() {
            WdfObjectDelete(objBag);
            });

        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagRetrieveGuid(objBag, &UniqueID, &uniqueId));

        RETURN_NTSTATUS_IF_FAILED(RtlStringFromGUID(uniqueId, &uniqueIdStr));

        // Init the deviceId unicode string.
        pnpDeviceId.Buffer = wstrBuffer;
        pnpDeviceId.Length = 0;
        pnpDeviceId.MaximumLength = (USHORT)(sizeof(WCHAR) * wstrBufferCch);

        status = RtlUnicodeStringPrintf(&pnpDeviceId, RENDER_DEVICE_ID_STR, &uniqueIdStr);

        RtlFreeUnicodeString(&uniqueIdStr);

        RETURN_NTSTATUS_IF_FAILED(status);

        // This is the device ID and the first H/W ID. 
        // This ID is used to create a unique audio device interface.
        // Note that this ID is NOT the match with this driver's INF.
        RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignDeviceID(devInit, &pnpDeviceId));

        RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddHardwareID(devInit, &pnpDeviceId));
    }

    // This H/W ID is the match with this driver's INF.
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddHardwareID(devInit, &RenderHardwareId));

    /*
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddCompatibleID(devInit, &RenderCompatibleId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignInstanceID(devInit, &RenderInstanceId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignContainerID(devInit, &RenderContainerId));

    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddDeviceText(devInit,
                                     &RenderDeviceLocation,
                                     &RenderDeviceLocation,
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
    pnpPowerCallbacks.EvtDevicePrepareHardware = DspR_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = DspR_EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = DspR_EvtDeviceSelfManagedIoInit;
    WdfDeviceInitSetPnpPowerEventCallbacks(devInit, &pnpPowerCallbacks);

    //
    // Specify a context for this render device.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_RENDER_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = DspR_EvtDeviceContextCleanup;
    attributes.ExecutionLevel = WdfExecutionLevelPassive;

    WDFDEVICE device = NULL;
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&devInit, &attributes, &device));

    devInit_free.release();

    //
    // Init render's device context.
    //
    PDSP_RENDER_DEVICE_CONTEXT devCtx;
    devCtx = GetRenderDeviceContext(device);
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

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCA VDSP Parent %p Create Circuit Device %p", Parent, device);

    *Device = device;

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


#pragma code_seg("PAGE")
NTSTATUS
DspR_EvtProcessCommand0(
    _In_    ACXAUDIOMODULE  AudioModule,
    _In_    PVOID           InBuffer,
    _In_    ULONG           InBufferCb,
    _In_    PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferCb
   )
{
    BOOL                            fNewValue = FALSE;
    PVOID                           currentValue = nullptr;
    PVOID                           inBuffer = nullptr;
    ULONG                           inBufferCb = 0;
    PDSP_AUDIOMODULE0_CONTEXT       audioModuleCtx;
    AUDIOMODULE_PARAMETER_INFO *    parameterInfo = nullptr;
    AUDIOMODULE_CUSTOM_COMMAND *    command = nullptr;
    
    PAGED_CODE();

    audioModuleCtx = GetDspAudioModule0Context(AudioModule);
    RETURN_NTSTATUS_IF_TRUE(nullptr == audioModuleCtx, STATUS_INTERNAL_ERROR);

    //
    // Basic parameter validation (module specific).
    //
    RETURN_NTSTATUS_IF_TRUE(InBuffer == nullptr || InBufferCb == 0, STATUS_INVALID_PARAMETER);
    RETURN_NTSTATUS_IF_TRUE(InBufferCb < sizeof(AUDIOMODULE_CUSTOM_COMMAND), STATUS_INVALID_PARAMETER);

    command = (AUDIOMODULE_CUSTOM_COMMAND*)InBuffer;  

    RETURN_NTSTATUS_IF_TRUE(command->ParameterId >= SIZEOF_ARRAY(AudioModule0_ParameterInfo), STATUS_INVALID_PARAMETER);

    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModuleParameter1:
            currentValue = &audioModuleCtx->Parameter1;
            parameterInfo = &AudioModule0_ParameterInfo[AudioModuleParameter1];
            break;
        case AudioModuleParameter2:
            currentValue = &audioModuleCtx->Parameter2;
            parameterInfo = &AudioModule0_ParameterInfo[AudioModuleParameter2];
            break;
        default:
            RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE_CUSTOM_COMMAND);

    if (inBufferCb == 0)
    {
        inBuffer = nullptr;
    }
    
    RETURN_NTSTATUS_IF_FAILED(AudioModule_GenericHandler(
                command->Verb,
                command->ParameterId,
                parameterInfo,
                currentValue,
                inBuffer,
                inBufferCb,
                OutBuffer,
                OutBufferCb,
                &fNewValue));
    
    if (fNewValue &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        AUDIOMODULE_CUSTOM_NOTIFICATION customNotification = {0};

        customNotification.Type = AudioModuleParameterChanged;
        customNotification.ParameterChanged.ParameterId = command->ParameterId;

        RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
    }

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
NTSTATUS
DspR_EvtProcessCommand1(
    _In_    ACXAUDIOMODULE  AudioModule,
    _In_    PVOID           InBuffer,
    _In_    ULONG           InBufferCb,
    _In_    PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferCb
   )
{
    BOOL                            fNewValue = FALSE;
    PVOID                           currentValue = nullptr;
    PVOID                           inBuffer = nullptr;
    ULONG                           inBufferCb = 0;
    PDSP_AUDIOMODULE1_CONTEXT       audioModuleCtx;
    AUDIOMODULE_PARAMETER_INFO *    parameterInfo = nullptr;
    AUDIOMODULE_CUSTOM_COMMAND *    command = nullptr;
    
    PAGED_CODE();

    audioModuleCtx = GetDspAudioModule1Context(AudioModule);
    RETURN_NTSTATUS_IF_TRUE(nullptr == audioModuleCtx, STATUS_INTERNAL_ERROR);

    //
    // Basic parameter validation (module specific).
    //
    RETURN_NTSTATUS_IF_TRUE(InBuffer == nullptr || InBufferCb == 0, STATUS_INVALID_PARAMETER);
    RETURN_NTSTATUS_IF_TRUE(InBufferCb < sizeof(AUDIOMODULE_CUSTOM_COMMAND), STATUS_INVALID_PARAMETER);

    command = (AUDIOMODULE_CUSTOM_COMMAND*)InBuffer;  

    RETURN_NTSTATUS_IF_TRUE(command->ParameterId >= SIZEOF_ARRAY(AudioModule1_ParameterInfo), STATUS_INVALID_PARAMETER);

    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModuleParameter1:
            currentValue = &audioModuleCtx->Parameter1;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModuleParameter1];
            break;
        case AudioModuleParameter2:
            currentValue = &audioModuleCtx->Parameter2;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModuleParameter2];
            break;
        case AudioModuleParameter3:
            currentValue = &audioModuleCtx->Parameter3;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModuleParameter3];
            break;
        default:
            RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE_CUSTOM_COMMAND);

    if (inBufferCb == 0)
    {
        inBuffer = nullptr;
    }
    
    RETURN_NTSTATUS_IF_FAILED(AudioModule_GenericHandler(
                command->Verb,
                command->ParameterId,
                parameterInfo,
                currentValue,
                inBuffer,
                inBufferCb,
                OutBuffer,
                OutBufferCb,
                &fNewValue));
    
    if (fNewValue &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        AUDIOMODULE_CUSTOM_NOTIFICATION customNotification = {0};

        customNotification.Type = AudioModuleParameterChanged;
        customNotification.ParameterChanged.ParameterId = command->ParameterId;

        RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
    }

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
NTSTATUS
DspR_EvtProcessCommand2(
    _In_    ACXAUDIOMODULE  AudioModule,
    _In_    PVOID           InBuffer,
    _In_    ULONG           InBufferCb,
    _In_    PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferCb
   )
{
    BOOL                            fNewValue = FALSE;
    PVOID                           currentValue = nullptr;
    PVOID                           inBuffer = nullptr;
    ULONG                           inBufferCb = 0;
    PDSP_AUDIOMODULE2_CONTEXT       audioModuleCtx;
    AUDIOMODULE_PARAMETER_INFO *    parameterInfo = nullptr;
    AUDIOMODULE_CUSTOM_COMMAND *    command = nullptr;
    
    PAGED_CODE();

    audioModuleCtx = GetDspAudioModule2Context(AudioModule);
    RETURN_NTSTATUS_IF_TRUE(nullptr == audioModuleCtx, STATUS_INTERNAL_ERROR);

    //
    // Basic parameter validation (module specific).
    //
    RETURN_NTSTATUS_IF_TRUE(InBuffer == nullptr || InBufferCb == 0, STATUS_INVALID_PARAMETER);
    RETURN_NTSTATUS_IF_TRUE(InBufferCb < sizeof(AUDIOMODULE_CUSTOM_COMMAND), STATUS_INVALID_PARAMETER);

    command = (AUDIOMODULE_CUSTOM_COMMAND*)InBuffer;  

    RETURN_NTSTATUS_IF_TRUE(command->ParameterId >= SIZEOF_ARRAY(AudioModule2_ParameterInfo), STATUS_INVALID_PARAMETER);

    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModuleParameter1:
            currentValue = &audioModuleCtx->Parameter1;
            parameterInfo = &AudioModule2_ParameterInfo[AudioModuleParameter1];
            break;
        case AudioModuleParameter2:
            currentValue = &audioModuleCtx->Parameter2;
            parameterInfo = &AudioModule2_ParameterInfo[AudioModuleParameter2];
            break;
        default:
            RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE_CUSTOM_COMMAND);

    if (inBufferCb == 0)
    {
        inBuffer = nullptr;
    }
    
    RETURN_NTSTATUS_IF_FAILED(AudioModule_GenericHandler(
                command->Verb,
                command->ParameterId,
                parameterInfo,
                currentValue,
                inBuffer,
                inBufferCb,
                OutBuffer,
                OutBufferCb,
                &fNewValue));
    
    if (fNewValue &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        AUDIOMODULE_CUSTOM_NOTIFICATION customNotification = {0};

        customNotification.Type = AudioModuleParameterChanged;
        customNotification.ParameterChanged.ParameterId = command->ParameterId;

        RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
    }

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
NTSTATUS
DspR_CreateCircuitModules(
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit
    )
/*++

Routine Description:

    This routine creates all of the audio module elements and adds them to the circuit

Return Value:

    NT status value

--*/
{
    WDF_OBJECT_ATTRIBUTES           attributes;
    ACX_AUDIOMODULE_CALLBACKS       audioModuleCallbacks;
    ACX_AUDIOMODULE_CONFIG          audioModuleCfg;
    ACXAUDIOMODULE                  audioModuleElement;
    PDSP_AUDIOMODULE0_CONTEXT       audioModule0Ctx;
    PDSP_AUDIOMODULE1_CONTEXT       audioModule1Ctx;
    PDSP_AUDIOMODULE2_CONTEXT       audioModule2Ctx;
    ACX_PNPEVENT_CONFIG             audioModuleEventCfg;
    ACXPNPEVENT                     audioModuleEvent;

    PAGED_CODE();

    // Now add audio modules to the circuit
    // module 0

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = DspR_EvtProcessCommand0;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule0Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule0Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(0,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE0_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE0_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE0DESCRIPTION,
                wcslen(AUDIOMODULE0DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_AUDIOMODULE0_CONTEXT);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Circuit, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule0Ctx = GetDspAudioModule0Context(audioModuleElement);
    ASSERT(audioModule0Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule0Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(Circuit, (ACXELEMENT *) &audioModuleElement, 1));

    // module 1

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = DspR_EvtProcessCommand1;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule1Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule1Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(0,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE1_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE1_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE1DESCRIPTION,
                wcslen(AUDIOMODULE1DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_AUDIOMODULE1_CONTEXT);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Circuit, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule1Ctx = GetDspAudioModule1Context(audioModuleElement);
    ASSERT(audioModule1Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule1Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(Circuit, (ACXELEMENT *) &audioModuleElement, 1));

    // module 2

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = DspR_EvtProcessCommand2;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule2Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule2Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(1,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE2_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE2_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE2DESCRIPTION,
                wcslen(AUDIOMODULE2DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_AUDIOMODULE2_CONTEXT);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Circuit, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule2Ctx = GetDspAudioModule2Context(audioModuleElement);
    ASSERT(audioModule2Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule2Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(Circuit, (ACXELEMENT *) &audioModuleElement, 1));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
DspR_AddOffloadFormats(
    _In_ ACXPIN Pin
)
{
    PAGED_CODE();

    ACXCIRCUIT circuit = AcxPinGetCircuit(Pin);
    WDFDEVICE device = AcxCircuitGetWdfDevice(circuit);
    // PCM:44100 channel:2 24in32
    ACXDATAFORMAT formatPcm44100c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm44100c2_24in32, circuit, device, &formatPcm44100c2_24in32));

    // PCM:48000 channel:2 24in32
    ACXDATAFORMAT formatPcm48000c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm48000c2_24in32, circuit, device, &formatPcm48000c2_24in32));

    // PCM:96000 channel:2 24in32
    ACXDATAFORMAT formatPcm96000c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm96000c2_24in32, circuit, device, &formatPcm96000c2_24in32));

    // PCM:192000 channel:2 24in32
    ACXDATAFORMAT formatPcm192000c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm192000c2_24in32, circuit, device, &formatPcm192000c2_24in32));

    // PCM:44100 channel:2 16
    ACXDATAFORMAT formatPcm44100c2;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm44100c2, circuit, device, &formatPcm44100c2));

    // PCM:48000 channel:2 16
    ACXDATAFORMAT formatPcm48000c2;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm48000c2, circuit, device, &formatPcm48000c2));

    // PCM:96000 channel:2 16
    ACXDATAFORMAT formatPcm96000c2;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm96000c2, circuit, device, &formatPcm96000c2));

    // PCM:192000 channel:2 16
    ACXDATAFORMAT formatPcm192000c2;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm192000c2, circuit, device, &formatPcm192000c2));

    //
    // Add our supported formats to the raw mode for the circuit
    //
    ACXDATAFORMATLIST formatList = AcxPinGetRawDataFormatList(Pin);
    RETURN_NTSTATUS_IF_TRUE(formatList == NULL, STATUS_INSUFFICIENT_RESOURCES);

    //
    // For Offload scenarios, Windows will use 16 bit per sample offload only
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm48000c2));

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm96000c2));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2));

    // Include the formats supported by the host pin as well.
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm48000c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm96000c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2_24in32));

    //
    // Set up supported Default Mode formats
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = circuit;

    ACX_DATAFORMAT_LIST_CONFIG dflCfg;
    ACX_DATAFORMAT_LIST_CONFIG_INIT(&dflCfg);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListCreate(device, &attributes, &dflCfg, &formatList));

    //
    // For Offload scenarios, Windows will use 16 bit per sample offload only
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm48000c2));

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm96000c2));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2));

    // Include the formats supported by the host pin as well.
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm48000c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm96000c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2_24in32));

    RETURN_NTSTATUS_IF_FAILED(AcxPinAssignModeDataFormatList(Pin, &AUDIO_SIGNALPROCESSINGMODE_DEFAULT, formatList));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxFactoryCircuitCreateCircuit(
    _In_ WDFDEVICE                          Parent,
    _In_ WDFDEVICE                          Device,
    _In_ ACXFACTORYCIRCUIT                  Factory,
    _In_ PACX_FACTORY_CIRCUIT_ADD_CIRCUIT   CircuitConfig,
    _In_ PACXCIRCUIT_INIT                   CircuitInit,
    _In_ ULONG                              DataPortNumber,
    _In_opt_ PSDCA_PATH_DESCRIPTORS2        PathDescriptors
)
{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Factory);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVDspLog);

    DECLARE_CONST_UNICODE_STRING(circuitName, L"Speaker0");

    WDF_OBJECT_ATTRIBUTES attributes;

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
    RETURN_NTSTATUS_IF_FAILED(CreateRenderCircuit(CircuitInit, circuitName, Device, &circuit));
    AcpiReader * acpiReader = GetAcpiReaderDeviceContext(Parent);

    RETURN_NTSTATUS_IF_FAILED(DetermineSpecialStreamDetailsFromVendorProperties(circuit, acpiReader, CircuitConfig->CircuitProperties));

    ASSERT(circuit != NULL);
    DSP_CIRCUIT_CONTEXT *circuitCtx;
    circuitCtx = GetDspCircuitContext(circuit);
    ASSERT(circuitCtx);

    circuitCtx->EndpointId = endpointId;
    circuitCtx->DataPortNumber = DataPortNumber;
    circuitCtx->IsRenderCircuit = TRUE;

    //
    // Sim Peakmeter
    //
    circuitCtx->peakMeter = (PVOID)new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CSimPeakMeter();
    RETURN_NTSTATUS_IF_TRUE(NULL == circuitCtx->peakMeter, STATUS_INSUFFICIENT_RESOURCES);

    //
    // Post circuit creation initialization.
    //

    ///////////////////////////////////////////////////////////
    //
    // Allocate the formats this circuit supports.
    //
    // PCM:44100 channel:2 24in32
    ACXDATAFORMAT formatPcm44100c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm44100c2_24in32, circuit, Device, &formatPcm44100c2_24in32));

    // PCM:48000 channel:2 24in32
    ACXDATAFORMAT formatPcm48000c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm48000c2_24in32, circuit, Device, &formatPcm48000c2_24in32));

    // PCM:96000 channel:2 24in32
    ACXDATAFORMAT formatPcm96000c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm96000c2_24in32, circuit, Device, &formatPcm96000c2_24in32));

    // PCM:192000 channel:2 24in32
    ACXDATAFORMAT formatPcm192000c2_24in32;
    RETURN_NTSTATUS_IF_FAILED(AllocateFormat(Pcm192000c2_24in32, circuit, Device, &formatPcm192000c2_24in32));

    ///////////////////////////////////////////////////////////
    //
    // Create Pins
    //
    ACXPIN pins[DspPinType_Count];

    //
    // Create host render pin.
    //

    ACX_PIN_CALLBACKS pinCallbacks;
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat = DspR_EvtAcxPinSetDataFormat;

    RETURN_NTSTATUS_IF_FAILED(CreatePin(AcxPinTypeSink,
                                        circuit,
                                        AcxPinCommunicationSink,
                                        &KSCATEGORY_AUDIO,
                                        &pinCallbacks,
                                        DSPR_MAX_INPUT_HOST_STREAMS,
                                        false,
                                        &pins[DspPinTypeHost]));
    ASSERT(pins[DspPinTypeHost] != NULL);

    PDSP_PIN_CONTEXT pinCtx;
    pinCtx = GetDspPinContext(pins[DspPinTypeHost]);
    ASSERT(pinCtx);
    pinCtx->PinType = DspPinTypeHost;

    //
    // A DSP driver could add the formats it supports here, or it could wait until
    // the downstream pin is connected and discover the supported formats to use
    // formats supported by the SdcaClass driver for this endpoint based on the
    // DisCo data for the endpoint (e.g. supported data port widths, supported clock
    // sample rates, etc.)
    //
    ACXDATAFORMATLIST formatList;
    formatList = AcxPinGetRawDataFormatList(pins[DspPinTypeHost]);
    RETURN_NTSTATUS_IF_TRUE(formatList == NULL, STATUS_INSUFFICIENT_RESOURCES);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm48000c2_24in32));

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2_24in32));

    //
    // Set up supported Default Mode formats
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = circuit;
    ACX_DATAFORMAT_LIST_CONFIG dflCfg;
    ACX_DATAFORMAT_LIST_CONFIG_INIT(&dflCfg);
    AcxDataFormatListCreate(Device, &attributes, &dflCfg, &formatList);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm48000c2_24in32));

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2_24in32));

    RETURN_NTSTATUS_IF_FAILED(AcxPinAssignModeDataFormatList(pins[DspPinTypeHost], &AUDIO_SIGNALPROCESSINGMODE_DEFAULT, formatList));

    ///////////////////////////////////////////////////////////
    //
    // Create Offload Render Pin.
    //

    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat = DspR_EvtAcxPinSetDataFormat;

    RETURN_NTSTATUS_IF_FAILED(CreatePin(AcxPinTypeSink,
                                        circuit,
                                        AcxPinCommunicationSink,
                                        &KSCATEGORY_AUDIO,
                                        &pinCallbacks,
                                        DSPR_MAX_INPUT_OFFLOAD_STREAMS,
                                        false,
                                        &pins[DspPinTypeOffload]));
    ASSERT(pins[DspPinTypeOffload] != NULL);

    pinCtx = GetDspPinContext(pins[DspPinTypeOffload]);
    ASSERT(pinCtx);
    pinCtx->PinType = DspPinTypeOffload;

    RETURN_NTSTATUS_IF_FAILED(DspR_AddOffloadFormats(pins[DspPinTypeOffload]));

    ///////////////////////////////////////////////////////////
    //
    // Create loopback Pin.
    //

    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat = DspR_EvtAcxPinSetDataFormat;

    RETURN_NTSTATUS_IF_FAILED(CreatePin(AcxPinTypeSource,
                                        circuit,
                                        AcxPinCommunicationSink,
                                        &KSNODETYPE_AUDIO_LOOPBACK,
                                        &pinCallbacks,
                                        DSPR_MAX_OUTPUT_LOOPBACK_STREAMS,
                                        false,
                                        &pins[DspPinTypeLoopback]));
    ASSERT(pins[DspPinTypeLoopback] != NULL);

    pinCtx = GetDspPinContext(pins[DspPinTypeLoopback]);
    ASSERT(pinCtx);
    pinCtx->PinType = DspPinTypeLoopback;

    //
    // Add our supported formats to the raw mode for the circuit
    //
    formatList = AcxPinGetRawDataFormatList(pins[DspPinTypeLoopback]);
    RETURN_NTSTATUS_IF_TRUE(formatList == NULL, STATUS_INSUFFICIENT_RESOURCES);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm48000c2_24in32));

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2_24in32));

    //
    // Create Audio Engine
    //
    ACXAUDIOENGINE audioEngineElement;
    RETURN_NTSTATUS_IF_FAILED(CreateAudioEngine(circuit, pins, &audioEngineElement));
    circuitCtx->AudioEngineElement = audioEngineElement;

    PDSP_ENGINE_CONTEXT audioEngineCtx;
    audioEngineCtx = GetDspEngineContext(audioEngineElement);

    //
    // Add our supported formats to the audio engine device format list
    //
    formatList = AcxAudioEngineGetDeviceFormatList(audioEngineElement);
    RETURN_NTSTATUS_IF_TRUE(formatList == NULL, STATUS_INSUFFICIENT_RESOURCES);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm48000c2_24in32));

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2_24in32));

    // Create a new format to use for Engine Mix format
    AllocateFormat(Pcm48000c2_24in32, circuit, Device, &formatPcm48000c2_24in32);
    audioEngineCtx->MixFormat = formatPcm48000c2_24in32;

    // Set the global efects as disabled
    audioEngineCtx->GFxEnabled = FALSE;

    //
    // Add AudioEngine to the circuit
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(circuit, (ACXELEMENT*)&audioEngineElement, 1));

    //
    // Create and add the audio modules
    //
    RETURN_NTSTATUS_IF_FAILED(DspR_CreateCircuitModules(Device, circuit));

    ///////////////////////////////////////////////////////////
    //
    // Create bridge pin.
    //
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinConnected = DspR_EvtPinConnected;
    pinCallbacks.EvtAcxPinDisconnected = DspR_EvtPinDisconnected;

    RETURN_NTSTATUS_IF_FAILED(CreatePin(AcxPinTypeSource,
                                        circuit,
                                        AcxPinCommunicationNone,
                                        &KSCATEGORY_AUDIO,
                                        &pinCallbacks,
                                        0,
                                        false,
                                        &pins[DspPinTypeBridge]));
    ASSERT(pins[DspPinTypeBridge] != NULL);

    pinCtx = GetDspPinContext(pins[DspPinTypeBridge]);
    ASSERT(pinCtx);
    pinCtx->PinType = DspPinTypeBridge;

    //
    // Add our supported formats to the raw mode for the bridge pin.
    // This is required for ACX to retrieve Device format
    //
    formatList = AcxPinGetRawDataFormatList(pins[DspPinTypeBridge]);
    RETURN_NTSTATUS_IF_TRUE(formatList == NULL, STATUS_INSUFFICIENT_RESOURCES);

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAssignDefaultDataFormat(formatList, formatPcm48000c2_24in32));

    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm44100c2_24in32));
    RETURN_NTSTATUS_IF_FAILED(AcxDataFormatListAddDataFormat(formatList, formatPcm192000c2_24in32));

    if (PathDescriptors != nullptr && PathDescriptors->Size > 0)
    {
        circuitCtx->AggregatedPathDescriptors = (PSDCA_PATH_DESCRIPTORS2)ExAllocatePool2(POOL_FLAG_NON_PAGED, PathDescriptors->Size, DRIVER_TAG);
        if (circuitCtx->AggregatedPathDescriptors == nullptr)
        {
            RETURN_NTSTATUS_IF_FAILED(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlCopyMemory(circuitCtx->AggregatedPathDescriptors, PathDescriptors, PathDescriptors->Size);
    }

    //
    // Add a stream BRIDGE.
    //

    ACX_STREAM_BRIDGE_CONFIG streamCfg;
    ACX_STREAM_BRIDGE_CONFIG_INIT(&streamCfg);

    RETURN_NTSTATUS_IF_FAILED(CreateStreamBridge(streamCfg, circuit, pins[DspPinTypeBridge], pinCtx,  DataPortNumber, endpointId, PathDescriptors, true));

    //
    // Add bridge pin
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, pins, DspPinType_Count));

    RETURN_NTSTATUS_IF_FAILED(ConnectRenderCircuitElements(audioEngineElement, circuit));

    //
    // Store the circuit handle in the render device context.
    //
    PDSP_RENDER_DEVICE_CONTEXT renderDevCtx = NULL;
    renderDevCtx = GetRenderDeviceContext(Device);
    ASSERT(renderDevCtx);
    renderDevCtx->Circuit = circuit;
    renderDevCtx->FirstTimePrepareHardware = TRUE;

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCA VDSP Circuit Device %p Create Circuit %p", Device, circuit);

    return status;
}

#pragma code_seg()
_Use_decl_annotations_
NTSTATUS
DspR_EvtCircuitPowerUp (
    WDFDEVICE,
    ACXCIRCUIT,
    WDF_POWER_DEVICE_STATE
    )
{
    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
_Use_decl_annotations_
NTSTATUS
DspR_EvtCircuitPowerDown (
    WDFDEVICE  Device,
    ACXCIRCUIT Circuit,
    WDF_POWER_DEVICE_STATE TargetState
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(TargetState);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtCircuitCompositeCircuitInitialize(
    WDFDEVICE      Device,
    ACXCIRCUIT     Circuit,
    ACXOBJECTBAG   CircuitProperties
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(CircuitProperties);

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtCircuitCompositeInitialize(
    WDFDEVICE      Device,
    ACXCIRCUIT     Circuit,
    ACXOBJECTBAG   CompositeProperties
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(CompositeProperties);

    return status;
}

#pragma code_seg("PAGE")
NTSTATUS
DspR_CreateStreamModules(
    _In_ WDFDEVICE  Device,
    _In_ ACXSTREAM Stream
    )
/*++

Routine Description:

    This routine creates all of the audio module elements and adds them to the stream

Return Value:

    NT status value

--*/
{
    WDF_OBJECT_ATTRIBUTES           attributes;
    ACX_AUDIOMODULE_CALLBACKS       audioModuleCallbacks;
    ACX_AUDIOMODULE_CONFIG          audioModuleCfg;
    ACXAUDIOMODULE                  audioModuleElement;
    PDSP_AUDIOMODULE0_CONTEXT       audioModule0Ctx;
    PDSP_AUDIOMODULE1_CONTEXT       audioModule1Ctx;
    PDSP_AUDIOMODULE2_CONTEXT       audioModule2Ctx;
    ACX_PNPEVENT_CONFIG             audioModuleEventCfg;
    ACXPNPEVENT                     audioModuleEvent;

    PAGED_CODE();

    // Now add audio modules to the stream
    // module 0
    // for simplicity of the example, we implement the same modules on the stream as is
    // on the circuit
    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = DspR_EvtProcessCommand0;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule0Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule0Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(1,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE0_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE0_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE0DESCRIPTION,
                wcslen(AUDIOMODULE0DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_AUDIOMODULE0_CONTEXT);
    attributes.ParentObject = Stream;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Stream, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule0Ctx = GetDspAudioModule0Context(audioModuleElement);
    ASSERT(audioModule0Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule0Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(Stream, (ACXELEMENT *) &audioModuleElement, 1));

    // module 1

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = DspR_EvtProcessCommand1;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule1Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule1Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(1,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE1_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE1_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE1DESCRIPTION,
                wcslen(AUDIOMODULE1DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_AUDIOMODULE1_CONTEXT);
    attributes.ParentObject = Stream;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Stream, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule1Ctx = GetDspAudioModule1Context(audioModuleElement);
    ASSERT(audioModule1Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule1Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(Stream, (ACXELEMENT *) &audioModuleElement, 1));

    // module 2

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = DspR_EvtProcessCommand2;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule2Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule2Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(2,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE2_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE2_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE2DESCRIPTION,
                wcslen(AUDIOMODULE2DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_AUDIOMODULE2_CONTEXT);
    attributes.ParentObject = Stream;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Stream, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule2Ctx = GetDspAudioModule2Context(audioModuleElement);
    ASSERT(audioModule2Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule2Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(Stream, (ACXELEMENT *) &audioModuleElement, 1));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
DspR_EvtCircuitCreateStream(
    _In_    WDFDEVICE       Device,
    _In_    ACXCIRCUIT      Circuit,
    _In_    ACXPIN          Pin,
    _In_    PACXSTREAM_INIT StreamInit,
    _In_    ACXDATAFORMAT   DataFormat,
    _In_    const GUID*     SignalProcessingMode,
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

    PDSP_PIN_CONTEXT pinCtx = GetDspPinContext(Pin);
    ASSERT(pinCtx);

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

    // Check incorrect pin instantiation.
    RETURN_NTSTATUS_IF_TRUE_MSG(NULL == pinCtx, STATUS_INVALID_PARAMETER, L"Incorrect pin is being instantiated");
    RETURN_NTSTATUS_IF_TRUE_MSG(
        NULL == pinCtx ||
        (pinCtx->PinType != DspPinTypeHost &&
        pinCtx->PinType != DspPinTypeOffload &&
        pinCtx->PinType != DspPinTypeLoopback),
        STATUS_INVALID_PARAMETER, L"Incorrect pin is being instantiated");

    //
    // TEST sending KS Property to connected circuits
    //
    ULONG testValue = 7;
    status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST1,
        AcxPropertyVerbSet,
        nullptr, 0,
        &testValue, sizeof(ULONG),
        nullptr);
    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCAVAD_PROPERTY_TEST1 SET :%!STATUS!, Value = %d", status, testValue);

    status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST2,
        AcxPropertyVerbGet,
        nullptr, 0,
        &testValue, sizeof(ULONG),
        nullptr);
    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCAVAD_PROPERTY_TEST2 GET :%!STATUS!, Value = %d", status, testValue);

    testValue = 8;
    status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST3,
        AcxPropertyVerbSet,
        nullptr, 0,
        &testValue, sizeof(ULONG),
        nullptr);
    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCAVAD_PROPERTY_TEST3 SET :%!STATUS!, Value = %d", status, testValue);

    status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST4,
        AcxPropertyVerbGet,
        nullptr, 0,
        &testValue, sizeof(ULONG),
        nullptr);
    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCAVAD_PROPERTY_TEST4 GET :%!STATUS!, Value = %d", status, testValue);

    testValue = 9;
    status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST5,
        AcxPropertyVerbSet,
        nullptr, 0,
        &testValue, sizeof(ULONG),
        nullptr);
    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCAVAD_PROPERTY_TEST5 SET :%!STATUS!, Value = %d", status, testValue);

    status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST6,
        AcxPropertyVerbGet,
        nullptr, 0,
        &testValue, sizeof(ULONG),
        nullptr);
    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"SDCAVAD_PROPERTY_TEST6 GET :%!STATUS!, Value = %d", status, testValue);

    status = STATUS_SUCCESS;

    if (pinCtx->PinType != DspPinTypeOffload)
    {
        //
        // Set circuit-callbacks.
        //
        RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxRequestPreprocessCallback(
            StreamInit,
            DspR_EvtStreamRequestPreprocess,
            (ACXCONTEXT)AcxRequestTypeAny, // dbg only
            AcxRequestTypeAny,
            NULL,
            AcxItemIdNone));
    }

    //
    // Request a Vendor-Specific property from the Controller
    //
    Dsp_SendVendorSpecificProperties(
        Device,
        Circuit,
        TRUE);

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
    streamCallbacks.EvtAcxStreamPrepareHardware = Dsp_EvtStreamPrepareHardware;
    streamCallbacks.EvtAcxStreamReleaseHardware = Dsp_EvtStreamReleaseHardware;
    streamCallbacks.EvtAcxStreamRun = Dsp_EvtStreamRun;
    streamCallbacks.EvtAcxStreamPause = Dsp_EvtStreamPause;
    streamCallbacks.EvtAcxStreamAssignDrmContentId = Dsp_EvtStreamAssignDrmContentId;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxStreamCallbacks(StreamInit, &streamCallbacks));

    //
    // Init RT streaming callbacks.
    //
    ACX_RT_STREAM_CALLBACKS rtCallbacks;
    ACX_RT_STREAM_CALLBACKS_INIT(&rtCallbacks);
    rtCallbacks.EvtAcxStreamGetHwLatency = Dsp_EvtStreamGetHwLatency;
    rtCallbacks.EvtAcxStreamAllocateRtPackets = Dsp_EvtStreamAllocateRtPackets;
    rtCallbacks.EvtAcxStreamFreeRtPackets = Dsp_EvtStreamFreeRtPackets;
    rtCallbacks.EvtAcxStreamSetRenderPacket = DspR_EvtStreamSetRenderPacket;
    rtCallbacks.EvtAcxStreamGetCurrentPacket = Dsp_EvtStreamGetCurrentPacket;
    rtCallbacks.EvtAcxStreamGetPresentationPosition = Dsp_EvtStreamGetPresentationPosition;

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

    PDSP_CIRCUIT_CONTEXT circuitCtx = GetDspCircuitContext(Circuit);
    ASSERT(circuitCtx);

    CStreamEngine* streamEngine = NULL;
    if (pinCtx->PinType == DspPinTypeOffload)
    {
        streamEngine = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) COffloadStreamEngine(stream, DataFormat, (CSimPeakMeter *)circuitCtx->peakMeter);
    }
    else
    {
        streamEngine = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CRenderStreamEngine(stream, DataFormat, (CSimPeakMeter *)circuitCtx->peakMeter);
    }
    RETURN_NTSTATUS_IF_TRUE(NULL == streamEngine, STATUS_INSUFFICIENT_RESOURCES);

    DSP_STREAM_CONTEXT* streamCtx;
    streamCtx = GetDspStreamContext(stream);
    ASSERT(streamCtx);
    streamCtx->StreamEngine = (PVOID)streamEngine;
    streamEngine = NULL;
    streamCtx->PinType = pinCtx->PinType;

    if (DspPinTypeLoopback == pinCtx->PinType &&
        circuitCtx->SpecialStreamAvailablePaths & SdcaPathReferenceStream)
    {
        WdfObjectReferenceWithTag(circuitCtx->SpecialStreamTargetCircuit, (PVOID)DRIVER_TAG);
        streamCtx->SpecialStreamTargetCircuit = circuitCtx->SpecialStreamTargetCircuit;
    }

    if ((DspPinTypeHost == pinCtx->PinType || DspPinTypeOffload == pinCtx->PinType) &&
        circuitCtx->SpecialStreamAvailablePaths & SdcaPathIvSense)
    {
        WdfObjectReferenceWithTag(circuitCtx->SpecialStreamTargetCircuit, (PVOID)DRIVER_TAG);
        streamCtx->SpecialStreamTargetCircuit = circuitCtx->SpecialStreamTargetCircuit;
    }

    //
    // Post stream creation initialization.
    //

    if (circuitCtx->AudioEngineElement != nullptr)
    {
        //
        // The circuit has an Audio Engine element, so all streams created for the circuit
        // also require an Audio Engine element to allow the OS to
        //  * Adjust per-stream volume and mute
        //  * Monitor per-stream peakmeter values
        //  * Retrieve stream position
        //  * Set stream effects state
        //

        //
        // Volume Element
        //
        ACX_VOLUME_CALLBACKS volumeCallbacks;
        ACX_VOLUME_CALLBACKS_INIT(&volumeCallbacks);
        volumeCallbacks.EvtAcxRampedVolumeAssignLevel = DspR_EvtRampedVolumeAssignLevel;
        volumeCallbacks.EvtAcxVolumeRetrieveLevel = DspR_EvtVolumeRetrieveLevel;

        // Create Volume element for the audio engine to use
        ACX_VOLUME_CONFIG volumeCfg;
        ACX_VOLUME_CONFIG_INIT(&volumeCfg);
        volumeCfg.ChannelsCount = MAX_CHANNELS;
        volumeCfg.Minimum = VOLUME_LEVEL_MINIMUM;
        volumeCfg.Maximum = VOLUME_LEVEL_MAXIMUM;
        volumeCfg.SteppingDelta = VOLUME_STEPPING;
        volumeCfg.Name = &KSAUDFNAME_VOLUME_CONTROL;
        volumeCfg.Callbacks = &volumeCallbacks;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_VOLUME_ELEMENT_CONTEXT);
        attributes.ParentObject = stream;

        ACXVOLUME volumeElement;
        RETURN_NTSTATUS_IF_FAILED(AcxVolumeCreate(stream, &attributes, &volumeCfg, &volumeElement));

        //
        // Mute Element
        //
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
        attributes.ParentObject = stream;

        ACXMUTE muteElement;
        RETURN_NTSTATUS_IF_FAILED(AcxMuteCreate(stream, &attributes, &muteCfg, &muteElement));

        //
        // Peakmeter Element
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
        attributes.ParentObject = stream;

        ACXPEAKMETER peakmeterElement;
        RETURN_NTSTATUS_IF_FAILED(AcxPeakMeterCreate(stream, &attributes, &peakmeterCfg, &peakmeterElement));

        PDSP_PEAKMETER_ELEMENT_CONTEXT peakmeterCtx;
        ASSERT(peakmeterElement != NULL);
        peakmeterCtx = GetDspPeakMeterElementContext(peakmeterElement);
        ASSERT(peakmeterCtx);
        peakmeterCtx->peakMeter = ((CStreamEngine*)streamCtx->StreamEngine)->GetPeakMeter();

        //
        // Stream Audio Engine Node
        //
        ACX_STREAMAUDIOENGINE_CALLBACKS streamAudioEngineCallbacks;
        // Create the AudioEngine element to control offloaded streaming.
        ACX_STREAMAUDIOENGINE_CALLBACKS_INIT(&streamAudioEngineCallbacks);
        streamAudioEngineCallbacks.EvtAcxStreamAudioEngineAssignEffectsState = DspR_EvtAcxStreamAudioEngineAssignEffectsState;
        streamAudioEngineCallbacks.EvtAcxStreamAudioEngineRetrieveEffectsState = DspR_EvtAcxStreamAudioEngineRetrieveEffectsState;
        streamAudioEngineCallbacks.EvtAcxStreamAudioEngineRetrievePresentationPosition = DspR_EvtAcxStreamAudioEngineRetrievePresentationPosition;
        streamAudioEngineCallbacks.EvtAcxStreamAudioEngineAssignCurrentWritePosition = DspR_EvtAcxStreamAudioEngineAssignCurrentWritePosition;
        streamAudioEngineCallbacks.EvtAcxStreamAudioEngineRetrieveLinearBufferPosition = DspR_EvtAcxStreamAudioEngineRetrieveLinearBufferPosition;
        streamAudioEngineCallbacks.EvtAcxStreamAudioEngineAssignLastBufferPosition = DspR_EvtAcxStreamAudioEngineAssignLastBufferPosition;
        streamAudioEngineCallbacks.EvtAcxStreamAudioEngineAssignLoopbackProtection = DspR_EvtAcxStreamAudioEngineAssignLoopbackProtection;

        ACX_STREAMAUDIOENGINE_CONFIG audioEngineCfg;
        ACX_STREAMAUDIOENGINE_CONFIG_INIT(&audioEngineCfg);
        audioEngineCfg.VolumeElement = volumeElement;
        audioEngineCfg.MuteElement = muteElement;
        audioEngineCfg.PeakMeterElement = peakmeterElement;
        audioEngineCfg.Callbacks = &streamAudioEngineCallbacks;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_STREAMAUDIOENGINE_CONTEXT);
        attributes.ParentObject = stream;

        ACXELEMENT streamAudioEngine;
        RETURN_NTSTATUS_IF_FAILED(AcxStreamAudioEngineCreate(stream, circuitCtx->AudioEngineElement, &attributes, &audioEngineCfg, (ACXSTREAMAUDIOENGINE*)&streamAudioEngine));

        // Set local effects as disabled
        PDSP_STREAMAUDIOENGINE_CONTEXT pStreamAudioEngineCtx;
        pStreamAudioEngineCtx = GetDspStreamAudioEngineContext(streamAudioEngine);
        pStreamAudioEngineCtx->LFxEnabled = FALSE;


        RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(stream, &streamAudioEngine, 1));

        // Add our stream audio modules
        RETURN_NTSTATUS_IF_FAILED(DspR_CreateStreamModules(Device, stream));
    }
    else
    {
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
        DSP_ELEMENT_CONTEXT* elementCtx;
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

        // Add our stream audio modules
        RETURN_NTSTATUS_IF_FAILED(DspR_CreateStreamModules(Device, stream));
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
NTSTATUS
DspR_EvtStreamSetRenderPacket(
    _In_ ACXSTREAM          Stream,
    _In_ ULONG              Packet,
    _In_ ULONG              Flags,
    _In_ ULONG              EosPacketLength
    )
{
    PDSP_STREAM_CONTEXT   ctx;
    CRenderStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = static_cast<CRenderStreamEngine*>(ctx->StreamEngine);

    return streamEngine->SetRenderPacket(Packet, Flags, EosPacketLength);
}

//
//#pragma code_seg()
//NTSTATUS
//DspR_EvtAcxCircuitProcess(
//    _In_ ACXCIRCUIT         Circuit, 
//    _In_ ACXSTREAMIO        Stream 
//    )
//{
//    UNREFERENCED_PARAMETER(Circuit);
//    UNREFERENCED_PARAMETER(Stream);
//    
//    return STATUS_SUCCESS;
//}
//


