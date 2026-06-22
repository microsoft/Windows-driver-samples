/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Device.cpp

Abstract:

    Plug and Play module. This file contains routines to handle pnp requests.

Environment:

    Kernel mode

--*/

#include "private.h"
#include "streamengine.h"
#include "AcpiReader.h"
#include <devguid.h>


#ifndef __INTELLISENSE__
#include "device.tmh"
#endif

using namespace ACPIREADER;

UNICODE_STRING g_RegistryPath = {0};      // This is used to store the registry settings path for the driver

DEFINE_GUID(DSP_CIRCUIT_RENDER_GUID, 
0x9e4f4968, 0x4dd0, 0x4aaa, 0x93, 0x0e, 0xcd, 0xc4, 0xe2, 0x8f, 0xf5, 0xb1);

DEFINE_GUID(DSP_CIRCUIT_CAPTURE_GUID,
0xe813215a, 0xfb5e, 0x4c9d, 0xb8, 0x99, 0x91, 0x18, 0x56, 0xb6, 0xde, 0x81);

// {17F5B19F-C2C7-4B53-AFB9-49A0283D0DCE}
DEFINE_GUID(DSP_CIRCUIT_SPEAKER_GUID,
    0x17f5b19f, 0xc2c7, 0x4b53, 0xaf, 0xb9, 0x49, 0xa0, 0x28, 0x3d, 0xd, 0xce);

// {6F9EACF7-CD2D-4030-9E49-7CC4ADEFF192}
DEFINE_GUID(DSP_CIRCUIT_MICROPHONE_GUID,
    0x6f9eacf7, 0xcd2d, 0x4030, 0x9e, 0x49, 0x7c, 0xc4, 0xad, 0xef, 0xf1, 0x92);

// {9B5AEA69-F6E5-4BA3-9968-37FA548F5503}
DEFINE_GUID(DSP_CIRCUIT_UNIVERSALJACK_RENDER_GUID,
    0x9b5aea69, 0xf6e5, 0x4ba3, 0x99, 0x68, 0x37, 0xfa, 0x54, 0x8f, 0x55, 0x3);

// {3D405590-9368-4706-88E1-B69AD80C8969}
DEFINE_GUID(DSP_CIRCUIT_UNIVERSALJACK_CAPTURE_GUID,
    0x3d405590, 0x9368, 0x4706, 0x88, 0xe1, 0xb6, 0x9a, 0xd8, 0xc, 0x89, 0x69);

// {4DCB0606-6415-4A36-BDC5-9B1792117DC9}
DEFINE_GUID(DSP_FACTORY_GUID,
    0x4dcb0606, 0x6415, 0x4a36, 0xbd, 0xc5, 0x9b, 0x17, 0x92, 0x11, 0x7d, 0xc9);

DEFINE_GUID(SYSTEM_CONTAINER_GUID,
0x00000000, 0x0000, 0x0000, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);

// See description in private.h
#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
//
// Factory class method: KSMETHODSETID_AcxFactoryCircuit
//
#define STATIC_KSMETHODSETID_AcxFactoryCircuit\
    0xc09a3089L, 0x3eee, 0x47e0, 0xb9, 0x37, 0x4a, 0x74, 0x66, 0xae, 0xed, 0x6b
DEFINE_GUIDSTRUCT("c09a3089-3eee-47e0-b937-4a7466aeed6b", KSMETHODSETID_AcxFactoryCircuit);
#define KSMETHODSETID_AcxFactoryCircuit DEFINE_GUIDNAMED(KSMETHODSETID_AcxFactoryCircuit)

typedef enum {
    KSMETHOD_ACXFACTORYCIRCUIT_ADDCIRCUIT       = 1,
    KSMETHOD_ACXFACTORYCIRCUIT_REMOVECIRCUIT    = 2,
} KSMETHOD_ACXFACTORYCIRCUIT;
#endif  // ACX_WORKAROUND_ACXFACTORYCIRCUIT_01

#pragma code_seg()

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS
CopyRegistrySettingsPath(
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

Copies the following registry path to a global variable.

\REGISTRY\MACHINE\SYSTEM\ControlSetxxx\Services\<driver>\Parameters

Arguments:

RegistryPath - Registry path passed to DriverEntry

Returns:

NTSTATUS - SUCCESS if able to configure the framework

--*/

{
    PAGED_CODE();

    // Initializing the unicode string, so that if it is not allocated it will not be deallocated too.
    RtlInitUnicodeString(&g_RegistryPath, NULL);

    g_RegistryPath.MaximumLength = RegistryPath->Length + sizeof(WCHAR);

    g_RegistryPath.Buffer = (PWCH)ExAllocatePool2(POOL_FLAG_PAGED, g_RegistryPath.MaximumLength, DRIVER_TAG);

    if (g_RegistryPath.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // ExAllocatePool2 zeros memory.

    RtlAppendUnicodeToString(&g_RegistryPath, RegistryPath->Buffer);

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_AddAudioSensorsDevice(
    _In_ WDFCHILDLIST                                    DeviceList,
    _In_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER    IdentificationDescription,
    _In_ PWDFDEVICE_INIT                                 ChildInit
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES           attributes;
    PAUDIO_SENSORS_DEVICE_CONTEXT   audioSensorsDevCtx;
    PDSP_DEVICE_CONTEXT             dspDevCtx;
    WDFDEVICE                       sensorsDevice = nullptr;

    WDFDEVICE Device = WdfChildListGetDevice(DeviceList);

    DECLARE_CONST_UNICODE_STRING(buffer, L"SOUNDWIRE\\AUDIOSENSORS");
    DECLARE_UNICODE_STRING_SIZE(buffer2, 128);
    DECLARE_CONST_UNICODE_STRING(AudioSensorsDeviceText, L"Audio Sensors Device");

    PAGED_CODE();

    UNREFERENCED_PARAMETER(IdentificationDescription);

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddHardwareID(ChildInit, &buffer));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignDeviceID(ChildInit, &buffer));

    RETURN_NTSTATUS_IF_FAILED(RtlUnicodeStringPrintf(&buffer2, L"%08x", 12345));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignInstanceID(ChildInit, &buffer2));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddDeviceText(ChildInit, &AudioSensorsDeviceText, &AudioSensorsDeviceText, 0x409));

    WdfPdoInitSetDefaultLocale(ChildInit, 0x409);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, AUDIO_SENSORS_DEVICE_CONTEXT);
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&ChildInit, &attributes, &sensorsDevice));

    dspDevCtx = GetDspDeviceContext(Device);
    ASSERT(dspDevCtx!=NULL);

    dspDevCtx->AudioSensorsDevice = sensorsDevice;

    audioSensorsDevCtx = GetAudioSensorsDeviceContext(sensorsDevice);
    ASSERT(audioSensorsDevCtx != NULL);

    //
    // Set device capabilities.
    //
    {
        WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
        WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);

        pnpCaps.SurpriseRemovalOK = WdfTrue;
        pnpCaps.UniqueID = WdfFalse;

        WdfDeviceSetPnpCapabilities(sensorsDevice, &pnpCaps);
    }

    DrvLogInfo(g_SDCAVDspLog, FLAG_INIT, "Successfully Created Audio Sensors Device.");

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
Dsp_CreateChildList(
    _In_ WDFDEVICE Device
)
{
    NTSTATUS                                    status = STATUS_SUCCESS;
    WDF_CHILD_LIST_CONFIG                       config;
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER description;
    PDSP_DEVICE_CONTEXT                         dspDevCtx;

    PAGED_CODE();

    dspDevCtx = GetDspDeviceContext(Device);
    ASSERT(dspDevCtx != NULL);

    //
    // Init a new child list so that we can enumerate Audio Sensors PDO
    //
    WDF_CHILD_LIST_CONFIG_INIT(
        &config,
        sizeof(WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER),
        Dsp_AddAudioSensorsDevice // callback to create a child device.
        );

    RETURN_NTSTATUS_IF_FAILED(WdfChildListCreate(
        Device,
        &config,
        WDF_NO_OBJECT_ATTRIBUTES,
        &dspDevCtx->ChildList));

    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description, sizeof(description));
    RETURN_NTSTATUS_IF_FAILED(WdfChildListAddOrUpdateChildDescriptionAsPresent(
        dspDevCtx->ChildList,
        &description,
        NULL));

    DrvLogInfo(g_SDCAVDspLog, FLAG_INIT, "Successfully created new child list");

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtBusDeviceAdd(
    _In_    WDFDRIVER        Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device. All the software resources
    should be allocated in this callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Driver);

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = Dsp_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = Dsp_EvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Specify the type of context needed. 
    // Use default locking, i.e., none.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = Dsp_EvtDeviceContextCleanup;
    
    //
    // Allow ACX to add any pre-requirement it needs on this device.
    //
    ACX_DEVICEINIT_CONFIG devInitCfg;
    ACX_DEVICEINIT_CONFIG_INIT(&devInitCfg);

    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitInitialize(DeviceInit, &devInitCfg));
    
    //
    // Create the device.
    //
    WDFDEVICE device = NULL;
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&DeviceInit, &attributes, &device));

    //
    // Init Dsp's device context.
    //
    PDSP_DEVICE_CONTEXT devCtx;
    devCtx = GetDspDeviceContext(device);
    ASSERT(devCtx != NULL);
    devCtx->Render = NULL;
    devCtx->Capture = NULL;
    //
    // Allow ACX to add any post-requirement it needs on this device.
    //
    ACX_DEVICE_CONFIG devCfg;
    ACX_DEVICE_CONFIG_INIT(&devCfg);
    
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitialize(device, &devCfg));

    //
    // Tell the framework to set the SurpriseRemovalOK in the DeviceCaps so
    // that you don't get the popup in usermode (on Win2K) when you surprise
    // remove the device.
    //
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.SurpriseRemovalOK = WdfTrue;
    WdfDeviceSetPnpCapabilities(device, &pnpCaps);

    //
    // Default child list is owned by ACX and can only contain PDOs that
    // ACX is aware of so create new child list that will contain Audio Sensors PDO.
    //
    RETURN_NTSTATUS_IF_FAILED(Dsp_CreateChildList(device));

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtDevicePrepareHardware(
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
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);
    
    PDSP_DEVICE_CONTEXT devCtx;
    devCtx = GetDspDeviceContext(Device);
    ASSERT(devCtx != NULL);


    RETURN_NTSTATUS_IF_FAILED(Dsp_SetPowerPolicy(Device));
    
    RETURN_NTSTATUS_IF_FAILED(CSaveData::SetDeviceObject(WdfDeviceWdmGetDeviceObject(Device)));

    RETURN_NTSTATUS_IF_FAILED(CSaveData::InitializeWorkItems(WdfDeviceWdmGetDeviceObject(Device)));

    RETURN_NTSTATUS_IF_FAILED(CWaveReader::InitializeWorkItems(WdfDeviceWdmGetDeviceObject(Device)));

    RETURN_NTSTATUS_IF_FAILED(AcpiReader::_CreateAndInitialize(Device, g_SDCAVDspLog, DRIVER_TAG));

    //
    // Add a circuit factory that will handle all different devices 
    //
    if (!devCtx->Factory)
    {
        RETURN_NTSTATUS_IF_FAILED(Dsp_AddFactoryCircuit(Device));
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtDeviceReleaseHardware(
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
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PDSP_DEVICE_CONTEXT devCtx;
    devCtx = GetDspDeviceContext(Device);
    ASSERT(devCtx != NULL);

    //
    // Note that we don't remove the factory circuit here (AcxDeviceRemoveFactoryCircuit).
    // If the factory circuit is removed here, any circuit devices created through it could
    // be destroyed without ACX knowledge resulting in a Duplicate PDO bugcheck.
    //


    CSaveData::DestroyWorkItems();
    CWaveReader::DestroyWorkItems();

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_SetPowerPolicy(
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

    status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);

    return status;
}

#pragma code_seg()
VOID
Dsp_EvtDeviceContextCleanup(
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
    WDFDEVICE               device;
    PDSP_DEVICE_CONTEXT     devCtx;

    device = (WDFDEVICE)WdfDevice;
    devCtx = GetDspDeviceContext(device);
    ASSERT(devCtx != NULL);

    if (devCtx->Capture)
    {
        DspC_CircuitCleanup(devCtx->Capture);
        devCtx->Capture = NULL;
    }

    if (devCtx->AudioSensorsDevice)
    {
        devCtx->AudioSensorsDevice = nullptr;
    }
}

PAGED_CODE_SEG
NTSTATUS
Dsp_DetermineCircuitDetailsFromVendorProperties(
    _In_        AcpiReader *    Acpi,
    _In_        ACXOBJECTBAG    CircuitProperties,
    _Out_       PGUID           CircuitId,
    _Out_opt_   ULONG *         DataPortNumber = nullptr,
    _In_        ULONG           MaxPathDescriptors = 0,
    _Out_opt_   PSDCA_PATH_DESCRIPTORS2  PathDescriptors = nullptr
)
{
    DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(VendorPropertiesBlock);
    WDFMEMORY vendorPropertiesBlock = NULL;
    char* vendorPropertiesBuffer = NULL;
    ULONG vendorPropertiesSize;
    NTSTATUS status = STATUS_NOT_FOUND;

    PAGED_CODE();

    if (PathDescriptors != nullptr && MaxPathDescriptors == 0)
    {
        RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagRetrieveBlob(CircuitProperties, &VendorPropertiesBlock, NULL, &vendorPropertiesBlock));

    auto cleanup1 = scope_exit([&vendorPropertiesBlock] ()
    {
        if (vendorPropertiesBlock != NULL)
        {
            WdfObjectDelete(vendorPropertiesBlock);
            vendorPropertiesBlock = NULL;
        }
    });

    RETURN_NTSTATUS_IF_FAILED_UNLESS_ALLOWED(
        Acpi->GetPropertyString("acpi-vendor-config-type", ACPI_METHOD_SECTION_DEVICE_PROPERTIES,
                                vendorPropertiesBlock, NULL, 0, &vendorPropertiesSize),
                                STATUS_BUFFER_TOO_SMALL);

    vendorPropertiesBuffer = (char*)ExAllocatePool2(POOL_FLAG_PAGED, vendorPropertiesSize, DRIVER_TAG);

    auto cleanup2 = scope_exit([&vendorPropertiesBuffer] ()
    {
        if (vendorPropertiesBuffer != NULL)
        {
            ExFreePool(vendorPropertiesBuffer);
            vendorPropertiesBuffer = NULL;
        }
    });

    RETURN_NTSTATUS_IF_FAILED(
        Acpi->GetPropertyString("acpi-vendor-config-type", ACPI_METHOD_SECTION_DEVICE_PROPERTIES,
                                vendorPropertiesBlock, vendorPropertiesBuffer, vendorPropertiesSize, &vendorPropertiesSize));

    // use ACPI methods to parse for DataPortNumber
    if (DataPortNumber)
    {
        *DataPortNumber = 0;
    }
    if (PathDescriptors)
    {
        RtlZeroMemory(PathDescriptors, sizeof(*PathDescriptors) + (MaxPathDescriptors - 1) * sizeof(PathDescriptors->Descriptor[0]));
    }

    *CircuitId = NULL_GUID;

    // This code also assumes Data Port number based on type of endpoint, which is not correct
    // for real hardware. Data Port number should come from ACPI.
    if (sizeof("Streaming_Speaker") <= vendorPropertiesSize && sizeof("Streaming_Speaker") == RtlCompareMemory((PBYTE)vendorPropertiesBuffer, "Streaming_Speaker", sizeof("Streaming_Speaker")))
    {
        *CircuitId = DSP_CIRCUIT_SPEAKER_GUID;
        if (DataPortNumber)
        {
            // Speaker connects to DP 1
            *DataPortNumber = 1;
        }
        status = STATUS_SUCCESS;
    }
    else if (sizeof("Streaming_MicrophoneArray") <= vendorPropertiesSize && sizeof("Streaming_MicrophoneArray") == RtlCompareMemory((PBYTE)vendorPropertiesBuffer, "Streaming_MicrophoneArray", sizeof("Streaming_MicrophoneArray")))
    {
        *CircuitId = DSP_CIRCUIT_MICROPHONE_GUID;
        if (DataPortNumber)
        {
            // Raw capture path connects to DP 6
            *DataPortNumber = 6;
        }
        status = STATUS_SUCCESS;
    }
    else if (sizeof("Streaming_Headphones") <= vendorPropertiesSize && sizeof("Streaming_Headphones") == RtlCompareMemory((PBYTE)vendorPropertiesBuffer, "Streaming_Headphones", sizeof("Streaming_Headphones")))
    {
        *CircuitId = DSP_CIRCUIT_UNIVERSALJACK_RENDER_GUID;
        if (DataPortNumber)
        {
            // UAJ Output uses DP 3
            *DataPortNumber = 3;
        }
        status = STATUS_SUCCESS;
    }
    else if (sizeof("Streaming_LineOut") <= vendorPropertiesSize && sizeof("Streaming_LineOut") == RtlCompareMemory((PBYTE)vendorPropertiesBuffer, "Streaming_LineOut", sizeof("Streaming_LineOut")))
    {
        *CircuitId = DSP_CIRCUIT_UNIVERSALJACK_RENDER_GUID;
        if (DataPortNumber)
        {
            // UAJ Output uses DP 3
            *DataPortNumber = 3;
        }
        status = STATUS_SUCCESS;
    }
    else if (sizeof("Streaming_HeadsetOutput") <= vendorPropertiesSize && sizeof("Streaming_HeadsetOutput") == RtlCompareMemory((PBYTE)vendorPropertiesBuffer, "Streaming_HeadsetOutput", sizeof("Streaming_HeadsetOutput")))
    {
        *CircuitId = DSP_CIRCUIT_UNIVERSALJACK_RENDER_GUID;
        if (DataPortNumber)
        {
            // UAJ Output uses DP 3
            *DataPortNumber = 3;
        }
        status = STATUS_SUCCESS;
    }
    else if (sizeof("Streaming_Microphone") <= vendorPropertiesSize && sizeof("Streaming_Microphone") == RtlCompareMemory((PBYTE)vendorPropertiesBuffer, "Streaming_Microphone", sizeof("Streaming_Microphone")))
    {
        *CircuitId = DSP_CIRCUIT_UNIVERSALJACK_CAPTURE_GUID;
        if (DataPortNumber)
        {
            // UAJ Input uses DP 2
            *DataPortNumber = 2;
        }
        status = STATUS_SUCCESS;
    }
    else if (sizeof("Streaming_LineIn") <= vendorPropertiesSize && sizeof("Streaming_LineIn") == RtlCompareMemory((PBYTE)vendorPropertiesBuffer, "Streaming_LineIn", sizeof("Streaming_LineIn")))
    {
        *CircuitId = DSP_CIRCUIT_UNIVERSALJACK_CAPTURE_GUID;
        if (DataPortNumber)
        {
            // UAJ Input uses DP 2
            *DataPortNumber = 2;
        }
        status = STATUS_SUCCESS;
    }
    else if (sizeof("Streaming_HeadsetMic") <= vendorPropertiesSize && sizeof("Streaming_HeadsetMic") == RtlCompareMemory((PBYTE)vendorPropertiesBuffer, "Streaming_HeadsetMic", sizeof("Streaming_HeadsetMic")))
    {
        *CircuitId = DSP_CIRCUIT_UNIVERSALJACK_CAPTURE_GUID;
        if (DataPortNumber)
        {
            // UAJ Input uses DP 2
            *DataPortNumber = 2;
        }
        status = STATUS_SUCCESS;
    }

    //
    // The below code would be replaced in a real DSP driver (or modified to use vendor-specific properties)
    //
    ULONG vendorAggCount = 0;
    NTSTATUS aggCountStatus = Acpi->GetPropertyULong("acpi-vendor-mstest-aggregateddevice-count", ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &vendorAggCount);
    if (NT_SUCCESS(aggCountStatus) && vendorAggCount > 0 && vendorAggCount <= MaxPathDescriptors && PathDescriptors != nullptr)
    {
        //
        // This endpoint supports aggregation. If we find the necessary properties for SDCA_PATH_DESCRIPTORS2 for each aggregated device
        // we'll use the PathDescriptors for the endpoint. The PathDescriptors allows each aggregated device to use different channel masks.
        //
        const ULONG MAX_PROPERTY_SIZE = ARRAYSIZE("acpi-vendor-mstest-aggregateddevice-%d-dp-channel-mask");
        ULONG peripheralSuccessCount = 0;
        size_t descriptorsSize = sizeof(*PathDescriptors) + sizeof(PathDescriptors->Descriptor[0]) * (vendorAggCount - 1);

        for (ULONG i = 0; i < vendorAggCount && i < MAX_AGGREGATED_DEVICES; ++i)
        {
            char propertyName[MAX_PROPERTY_SIZE];

            PathDescriptors->Descriptor[i].Size = sizeof(PathDescriptors->Descriptor[1]);
            PathDescriptors->Descriptor[i].Version = SDCA_PATH_DESCRIPTOR2_VERSION_1;
            PathDescriptors->Descriptor[i].FunctionInformationId = i;
            PathDescriptors->Descriptor[i].DataPortMap = SdcaDataPortMapIndexA;
            PathDescriptors->Descriptor[i].DataPortConfig[0].Size = sizeof(SOUNDWIRE_DATAPORT_CONFIGURATION);
            // EndpointId will be supplied during CreateStreamBridge
            PathDescriptors->Descriptor[i].DataPortConfig[0].EndpointId = 0;
            PathDescriptors->Descriptor[i].DataPortConfig[0].Modes = SoundWireDataPortModeIsochronous;

            // Values from the vendor blob of a partner's DSP driver
            status = RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-aggregateddevice-%d-unique-id", i);
            if (!NT_SUCCESS(status))
            {
                break;
            }

            // We need to be able to match each Descriptor we find with a specific aggregated device. The aggregated device ordering at runtime
            // can be different, so we need to save the UniqueID for the audio function now.
            // At pin connection, we will discover the aggregated devices and replace the Uniquie ID with the appropriate FunctionInformationId
            status = Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &PathDescriptors->Descriptor[i].FunctionInformationId);
            if (!NT_SUCCESS(status))
            {
                break;
            }

            status = RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-aggregateddevice-%d-terminal-id", i);
            if (!NT_SUCCESS(status))
            {
                break;
            }

            status = Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &PathDescriptors->Descriptor[i].TerminalEntityId);
            if (!NT_SUCCESS(status))
            {
                break;
            }

            status = RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-aggregateddevice-%d-dp-number", i);
            if (!NT_SUCCESS(status))
            {
                break;
            }

            status = Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &PathDescriptors->Descriptor[i].DataPortConfig[0].DataPortNumber);
            if (!NT_SUCCESS(status))
            {
                break;
            }

            status = RtlStringCbPrintfA(propertyName, sizeof(propertyName), "acpi-vendor-mstest-aggregateddevice-%d-dp-channel-mask", i);
            if (!NT_SUCCESS(status))
            {
                break;
            }

            status = Acpi->GetPropertyULong(propertyName, ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &PathDescriptors->Descriptor[i].DataPortConfig[0].ChannelMask);
            if (!NT_SUCCESS(status))
            {
                break;
            }

            ++peripheralSuccessCount;
        }

        if (peripheralSuccessCount == vendorAggCount)
        {
            // Found all the data we wanted for each of the aggregated devices
            PathDescriptors->Size = (ULONG)descriptorsSize;
            PathDescriptors->Version = SDCA_PATH_DESCRIPTOR2_VERSION_1;
            PathDescriptors->DescriptorCount = vendorAggCount;
            PathDescriptors->SdcaPath = SdcaPathDefault;
        }

        // Ignore failures retrieving optional properties
        status = STATUS_SUCCESS;
    }

    ULONG vendorDataPortNumber = ULONG_MAX;
    ULONG vendorChannelMask = ULONG_MAX;
    ULONG vendorTerminalId = ULONG_MAX;


    // GetPropertyULong will leave the value as is (ULONG_MAX) if it isn't found
    Acpi->GetPropertyULong("acpi-vendor-mstest-device-terminal-id", ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &vendorTerminalId);
    Acpi->GetPropertyULong("acpi-vendor-mstest-device-dp-number", ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &vendorDataPortNumber);
    Acpi->GetPropertyULong("acpi-vendor-mstest-device-dp-channel-mask", ACPI_METHOD_SECTION_DEVICE_PROPERTIES, vendorPropertiesBlock, &vendorChannelMask);

    // DataPortNumber by itself is retained to validate back compat with systems that don't support the
    // new PathDescriptors2 structure
    if (DataPortNumber)
    {
        // Example vendor property for a streaming device
        if (vendorDataPortNumber != ULONG_MAX)
        {
            *DataPortNumber = vendorDataPortNumber;
        }
    }

    // Only fill out the PathDescriptors here if we didn't already fill it out with aggregated information
    if (PathDescriptors && PathDescriptors->Size == 0)
    {
        // Example code if the vendor values have been discovered for a single non-aggregated endpoint
        if ((vendorTerminalId != ULONG_MAX) && (vendorDataPortNumber != ULONG_MAX) && (vendorChannelMask != ULONG_MAX))
        {
            // We have enough information to fill out the PathDescriptors structure
            PathDescriptors->Size = sizeof(*PathDescriptors);
            PathDescriptors->Version = SDCA_PATH_DESCRIPTOR2_VERSION_1;
            PathDescriptors->SdcaPath = SdcaPathDefault;
            // EndpointId will be filled in later
            PathDescriptors->EndpointId = 0;
            PathDescriptors->DescriptorCount = 1;
            PathDescriptors->Descriptor[0].Size = sizeof(PathDescriptors->Descriptor[0]);
            PathDescriptors->Descriptor[0].Version = SDCA_PATH_DESCRIPTOR2_VERSION_1;
            PathDescriptors->Descriptor[0].FunctionInformationId = 0;
            PathDescriptors->Descriptor[0].TerminalEntityId = vendorTerminalId;
            // DataPortMap indicates which DPIndex entries are used, in this sample we'll only use
            // a single data port and that will be DPIndex_A.
            PathDescriptors->Descriptor[0].DataPortMap = SdcaDataPortMapIndexA;
            PathDescriptors->Descriptor[0].DataPortConfig[0].Size = sizeof(PathDescriptors->Descriptor[0].DataPortConfig[0]);
            PathDescriptors->Descriptor[0].DataPortConfig[0].DataPortNumber = vendorDataPortNumber;
            // The Descriptor-specific EndpointId is ignored
            PathDescriptors->Descriptor[0].DataPortConfig[0].EndpointId = 0;
            // Mode may be specified as something other than Isochronous depending on hardware and configuration
            PathDescriptors->Descriptor[0].DataPortConfig[0].Modes = SoundWireDataPortModeIsochronous;
            PathDescriptors->Descriptor[0].DataPortConfig[0].ChannelMask = vendorChannelMask;
        }
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtAcxFactoryCircuitCreateCircuitDevice(
    _In_  WDFDEVICE                         Parent,
    _In_  ACXFACTORYCIRCUIT                 Factory,
    _In_  PACX_FACTORY_CIRCUIT_ADD_CIRCUIT  CircuitConfig,
    _Out_ WDFDEVICE *                       Device
)
{
    ACXOBJECTBAG circuitProperties;

    DECLARE_CONST_ACXOBJECTBAG_DRIVER_PROPERTY_NAME(msft, CircuitId);

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;

    *Device = NULL;

    // Create object bag from the CircuitProperties
    ACX_OBJECTBAG_CONFIG propConfig;
    ACX_OBJECTBAG_CONFIG_INIT(&propConfig);
    propConfig.Handle = CircuitConfig->CircuitProperties;
    propConfig.Flags |= AcxObjectBagConfigOpenWithHandle;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagOpen(&attributes, &propConfig, &circuitProperties));

    auto cleanupPropConfig = scope_exit([=]() {
        WdfObjectDelete(circuitProperties);
        }
    );

    // Retrieve the intended Circuit ID from the object bag
    GUID circuitId;
    AcpiReader * acpiReader = GetAcpiReaderDeviceContext(Parent);

    RETURN_NTSTATUS_IF_TRUE(acpiReader == NULL, STATUS_INVALID_PARAMETER);

    RETURN_NTSTATUS_IF_FAILED(Dsp_DetermineCircuitDetailsFromVendorProperties(acpiReader, circuitProperties, &circuitId));

    // Call the appropriate CreateCircuitDevice based on the Circuit ID
    if (IsEqualGUID(circuitId, DSP_CIRCUIT_MICROPHONE_GUID) || IsEqualGUID(circuitId, DSP_CIRCUIT_UNIVERSALJACK_CAPTURE_GUID))
    {
        status = DspC_EvtAcxFactoryCircuitCreateCircuitDevice(Parent, Factory, CircuitConfig, Device);
    }
    else if (IsEqualGUID(circuitId, DSP_CIRCUIT_SPEAKER_GUID) || IsEqualGUID(circuitId, DSP_CIRCUIT_UNIVERSALJACK_RENDER_GUID))
    {
        status = DspR_EvtAcxFactoryCircuitCreateCircuitDevice(Parent, Factory, CircuitConfig, Device);
    }
    else
    {
        status = STATUS_NOT_SUPPORTED;
        DrvLogError(g_SDCAVDspLog, FLAG_INIT, L"Unexpected CircuitId %!GUID!, %!STATUS!", &circuitId, status);
    }

// See description in private.h
#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
    //
    // On success, cache this device info.
    //
    if (NT_SUCCESS(status))
    {
        status = Dsp_AddChildDeviceToCache(Factory, &CircuitConfig->CircuitUniqueId, *Device);
        if (!NT_SUCCESS(status))
        {
            DrvLogError(g_SDCAVDspLog, FLAG_INIT, 
                        L"Dsp_AddChildDeviceToCache(Factory=%p, ID=%!GUID!, WDFDEVICE=%p) failed, %!STATUS!", 
                        Factory, &CircuitConfig->CircuitUniqueId, *Device, status);

            WdfObjectDelete(*Device);
            *Device = NULL;
        }
    }
#endif

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtAcxFactoryCircuitCreateCircuit(
    _In_ WDFDEVICE                          Parent,
    _In_ WDFDEVICE                          Device,
    _In_ ACXFACTORYCIRCUIT                  Factory,
    _In_ PACX_FACTORY_CIRCUIT_ADD_CIRCUIT   CircuitConfig,
    _In_ PACXCIRCUIT_INIT                   CircuitInit
)
{
    ACXOBJECTBAG circuitProperties;

    DECLARE_CONST_ACXOBJECTBAG_DRIVER_PROPERTY_NAME(msft, CircuitId);

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVDspLog);

    // Create object bag from the CompositeProperties
    ACX_OBJECTBAG_CONFIG propConfig;
    ACX_OBJECTBAG_CONFIG_INIT(&propConfig);
    propConfig.Handle = CircuitConfig->CircuitProperties;
    propConfig.Flags |= AcxObjectBagConfigOpenWithHandle;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagOpen(&attributes, &propConfig, &circuitProperties));

    auto cleanupPropConfig = scope_exit([=]() {
        WdfObjectDelete(circuitProperties);
        }
    );

    // Retrieve the intended Circuit ID from the object bag
    GUID circuitId;
    ULONG dataPortNumber = 0;

    PSDCA_PATH_DESCRIPTORS2 pathDescriptors = (PSDCA_PATH_DESCRIPTORS2)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        sizeof(SDCA_PATH_DESCRIPTORS2) + sizeof(SDCA_PATH_DESCRIPTOR2)*(MAX_AGGREGATED_DEVICES-1),
        DRIVER_TAG);
    if (pathDescriptors == nullptr)
    {
        RETURN_NTSTATUS_IF_FAILED(STATUS_INSUFFICIENT_RESOURCES);
    }
    auto descriptors_free = scope_exit([&pathDescriptors]()
    {
        ExFreePool(pathDescriptors);
    });

    AcpiReader * acpiReader = GetAcpiReaderDeviceContext(Parent);

    RETURN_NTSTATUS_IF_TRUE(acpiReader == NULL, STATUS_INVALID_PARAMETER);

    RETURN_NTSTATUS_IF_FAILED(Dsp_DetermineCircuitDetailsFromVendorProperties(
        acpiReader,
        circuitProperties,
        &circuitId,
        &dataPortNumber,
        MAX_AGGREGATED_DEVICES,
        pathDescriptors));

    AcxCircuitInitSetComponentId(CircuitInit, &circuitId);

    // Call the appropriate CreateCircuitDevice based on the Circuit ID
    if (IsEqualGUID(circuitId, DSP_CIRCUIT_MICROPHONE_GUID) || IsEqualGUID(circuitId, DSP_CIRCUIT_UNIVERSALJACK_CAPTURE_GUID))
    {
        return DspC_EvtAcxFactoryCircuitCreateCircuit(Parent, Device, Factory, CircuitConfig, CircuitInit, dataPortNumber, pathDescriptors);
    }
    else if (IsEqualGUID(circuitId, DSP_CIRCUIT_SPEAKER_GUID) || IsEqualGUID(circuitId, DSP_CIRCUIT_UNIVERSALJACK_RENDER_GUID))
    {
        return DspR_EvtAcxFactoryCircuitCreateCircuit(Parent, Device, Factory, CircuitConfig, CircuitInit, dataPortNumber, pathDescriptors);
    }

    status = STATUS_NOT_SUPPORTED;
    DrvLogError(g_SDCAVDspLog, FLAG_INIT, L"Unexpected CircuitId %!GUID!, %!STATUS!", &circuitId, status);
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_AddFactoryCircuit(
    _In_ WDFDEVICE  Device
)
{
    PAGED_CODE();

    NTSTATUS                status      = STATUS_SUCCESS;
    PDSP_DEVICE_CONTEXT     devCtx      = GetDspDeviceContext(Device);
    PDSP_FACTORY_CONTEXT    factoryCtx  = NULL;

    ASSERT(devCtx != NULL);

    DECLARE_CONST_UNICODE_STRING(dspFactoryName, L"VirtualDspFactoryCircuit");
    DECLARE_CONST_UNICODE_STRING(dspFactoryUri, L"acpi:obj-path:\\_SB.PC00.HDAS");

    //
    // Get a FactoryCircuitInit structure.
    //
    PACXFACTORYCIRCUIT_INIT factoryInit = NULL;
    factoryInit = AcxFactoryCircuitInitAllocate(Device);

    //
    // Add factory identifiers.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxFactoryCircuitInitAssignComponentUri(factoryInit, &dspFactoryUri));
    RETURN_NTSTATUS_IF_FAILED(AcxFactoryCircuitInitAssignName(factoryInit, &dspFactoryName));

    //
    // Add properties, events and methods.
    //
// See description in private.h
#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxFactoryCircuitInitAssignAcxRequestPreprocessCallback(
                                            factoryInit, 
                                            Dsp_EvtFactoryRemoveCircuitRequestPreprocess,
                                            (ACXCONTEXT)Device,
                                            AcxRequestTypeMethod,
                                            &KSMETHODSETID_AcxFactoryCircuit, 
                                            KSMETHOD_ACXFACTORYCIRCUIT_REMOVECIRCUIT));
#endif

    //
    // Assign the circuit's operation-callbacks.
    //
    ACX_FACTORY_CIRCUIT_OPERATION_CALLBACKS operationCallbacks;
    ACX_FACTORY_CIRCUIT_OPERATION_CALLBACKS_INIT(&operationCallbacks);
    operationCallbacks.EvtAcxFactoryCircuitCreateCircuitDevice = Dsp_EvtAcxFactoryCircuitCreateCircuitDevice;
    operationCallbacks.EvtAcxFactoryCircuitCreateCircuit = Dsp_EvtAcxFactoryCircuitCreateCircuit;
    AcxFactoryCircuitInitSetOperationCallbacks(factoryInit, &operationCallbacks);

    //
    // Create the factory circuit.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    ACXFACTORYCIRCUIT factory;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_FACTORY_CONTEXT);
    attributes.ParentObject = Device;
    attributes.EvtCleanupCallback = Dsp_EvtFactoryContextCleanup;
    attributes.EvtDestroyCallback = Dsp_EvtFactoryContextDestroy;

    ASSERT(devCtx->Factory == NULL);
    RETURN_NTSTATUS_IF_FAILED(AcxFactoryCircuitCreate(Device, &attributes, &factoryInit, &factory));
    ASSERT(factory != NULL);

    factoryCtx = GetDspFactoryContext(factory);
    factoryCtx->Device = Device;

// See description in private.h
#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
    RETURN_NTSTATUS_IF_FAILED(Dsp_InitializeChildDevicesCache(factory));
#endif  // ACX_WORKAROUND_ACXFACTORYCIRCUIT_01

    //
    // Add circuit factory to device.
    // It will remain added until the Device is cleaned up by WDF due to removal.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddFactoryCircuit(Device, factory));
    devCtx->Factory = factory;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_SendTestPropertyTo(
    _In_ WDFDEVICE Device,
    _In_ ACXCIRCUIT Circuit,
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

    NTSTATUS status = STATUS_SUCCESS;

    if (Information)
    {
        *Information = 0;
    }

    PDSP_CIRCUIT_CONTEXT circuitCtx = GetDspCircuitContext(Circuit);

    ACXPIN pin;
    if (circuitCtx->IsRenderCircuit)
    {
        pin = AcxCircuitGetPinById(Circuit, DspPinTypeBridge);
    }
    else
    {
        pin = AcxCircuitGetPinById(Circuit, DspCapturePinTypeBridge);
    }
    ASSERT(pin);

    DSP_PIN_CONTEXT* pinCtx = GetDspPinContext(pin);
    ASSERT(pinCtx);

    RETURN_NTSTATUS_IF_TRUE(pinCtx->TargetCircuit == NULL, STATUS_INVALID_DEVICE_STATE);

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
    RETURN_NTSTATUS_IF_FAILED(WdfRequestCreate(&attributes, AcxTargetCircuitGetWdfIoTarget(pinCtx->TargetCircuit), &request));
    auto request_free = scope_exit([&request]()
    {
        WdfObjectDelete(request);
    });

    RETURN_NTSTATUS_IF_FAILED(AcxTargetCircuitFormatRequestForProperty(pinCtx->TargetCircuit, request, &requestParams));

    WDF_REQUEST_SEND_OPTIONS sendOptions;
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_SEC(5));

    RETURN_NTSTATUS_IF_TRUE(!WdfRequestSend(request, AcxTargetCircuitGetWdfIoTarget(pinCtx->TargetCircuit), &sendOptions), STATUS_INVALID_DEVICE_REQUEST);
    status = WdfRequestGetStatus(request);
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

    return status;
}

PAGED_CODE_SEG
VOID
Dsp_SendVendorSpecificProperties(
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ BOOLEAN SetValue
)
{
    VIRTUAL_STACK_VENDOR_SPECIFIC_CONTROL           control = { 0 };
    VIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA   data = { 0 };
    ULONG_PTR                                       info;

    PAGED_CODE();

    control.VendorSpecificId = VirtualStackVendorSpecificRequestGetTestData;
    control.VendorSpecificSize = sizeof(control);
    control.Data.DataPort = 0;
    control.Data.EndpointId = 0;

    NTSTATUS status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        KSPROPERTYSETID_Sdca,
        KSPROPERTY_SDCA_VENDOR_SPECIFIC,
        AcxPropertyVerbGet,
        &control,
        sizeof(control),
        nullptr,
        0,
        &info);

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"KSPROPERTY_SDCA_VENDOR_SPECIFIC GetTestData for size request returns %!STATUS! (%p)", status, (void*)info);

    status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        KSPROPERTYSETID_Sdca,
        KSPROPERTY_SDCA_VENDOR_SPECIFIC,
        AcxPropertyVerbGet,
        &control,
        sizeof(control),
        &data,
        sizeof(data),
        &info);

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"KSPROPERTY_SDCA_VENDOR_SPECIFIC GetTestData returns %#x : %#x, %!STATUS!", data.Test1, data.Test2, status);

    RtlZeroMemory(&control, sizeof(control));
    control.VendorSpecificId = VirtualStackVendorSpecificRequestSetTestConfig;
    control.VendorSpecificSize = sizeof(control);
    control.Config.IsScatterGather = SetValue;
    status = Dsp_SendTestPropertyTo(
        Device,
        Circuit,
        KSPROPERTYSETID_Sdca,
        KSPROPERTY_SDCA_VENDOR_SPECIFIC,
        AcxPropertyVerbSet,
        &control,
        sizeof(control),
        &data,
        sizeof(data),
        &info);

    DrvLogInfo(g_SDCAVDspLog, FLAG_STREAM, L"KSPROPERTY_SDCA_VENDOR_SPECIFIC SetTestParam returned %!STATUS!", status);
}

PAGED_CODE_SEG
VOID
Dsp_EvtFactoryContextCleanup(
    _In_ WDFOBJECT      Factory
    )
{
    PAGED_CODE();

// See description in private.h
#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
    Dsp_CleanupChildDevicesCache((ACXFACTORYCIRCUIT)Factory);
#else
    UNREFERENCED_PARAMETER(Factory);
#endif
}

PAGED_CODE_SEG
VOID
Dsp_EvtFactoryContextDestroy(
    _In_ WDFOBJECT      Factory
    )
{
    PAGED_CODE();

// See description in private.h
#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
    Dsp_DeleteChildDevicesCache((ACXFACTORYCIRCUIT)Factory);
#else
    UNREFERENCED_PARAMETER(Factory);
#endif
}

// See description in private.h
#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
PAGED_CODE_SEG
NTSTATUS
Dsp_InitializeChildDevicesCache(
    _In_ ACXFACTORYCIRCUIT Factory
    )
{
    PDSP_FACTORY_CONTEXT    factoryCtx  = GetDspFactoryContext(Factory);

    PAGED_CODE();

    RETURN_NTSTATUS_IF_FAILED(WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &factoryCtx->CacheLock));
    RETURN_NTSTATUS_IF_FAILED(WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES, &factoryCtx->Cache));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
VOID
Dsp_CleanupChildDevicesCache(
    _In_ ACXFACTORYCIRCUIT  Factory
    )
{
    PDSP_FACTORY_CONTEXT    factoryCtx  = GetDspFactoryContext(Factory);
    WDFOBJECT               child       = NULL;

    PAGED_CODE();

    //
    // Factory is going away, cleanup child devices cache.
    //
    if (factoryCtx->Cache == NULL || factoryCtx->CacheLock == NULL)
    {
        return; // Nothing to do.
    }

    WdfWaitLockAcquire(factoryCtx->CacheLock, NULL);

    while ((child = WdfCollectionGetFirstItem(factoryCtx->Cache)) != NULL)
    {
        PDSP_DEVICEID_CONTEXT idCtx = GetDspDeviceIdContext(child);

        //
        // - zero out ID.
        // - remove the item from the cache.
        //
        idCtx->UniqueID = NULL_GUID;
        WdfCollectionRemoveItem(factoryCtx->Cache, 0);
    }

    WdfWaitLockRelease(factoryCtx->CacheLock);
}

PAGED_CODE_SEG
VOID
Dsp_DeleteChildDevicesCache(
    _In_ ACXFACTORYCIRCUIT  Factory
    )
{
    PDSP_FACTORY_CONTEXT factoryCtx = GetDspFactoryContext(Factory);

    PAGED_CODE();

    if (factoryCtx->Cache != NULL)
    {
        WdfObjectDelete(factoryCtx->Cache);
        factoryCtx->Cache = NULL;
    }

    if (factoryCtx->CacheLock != NULL)
    {
        WdfObjectDelete(factoryCtx->CacheLock);
        factoryCtx->CacheLock = NULL;
    }
}

PAGED_CODE_SEG
bool
Dsp_IsChildDeviceInCacheLocked(
    _In_ ACXFACTORYCIRCUIT  Factory,
    _In_ const GUID *       UniqueId
    )
{
    PDSP_FACTORY_CONTEXT    factoryCtx  = GetDspFactoryContext(Factory);
    ULONG                   count       = WdfCollectionGetCount(factoryCtx->Cache);
    bool                    isPresent   = false;

    PAGED_CODE();

    for (ULONG i = 0; i < count; i++)
    {
        WDFDEVICE               child   = NULL;
        PDSP_DEVICEID_CONTEXT   idCtx   = NULL;

        child = (WDFDEVICE)WdfCollectionGetItem(factoryCtx->Cache, i);
        idCtx = GetDspDeviceIdContext(child);

        if ((idCtx != 0) && IsEqualGUID(idCtx->UniqueID, *UniqueId))
        {
            // Found it.
            isPresent = true;
            break;
        }
    }

    return isPresent;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_AddChildDeviceToCache(
    _In_ ACXFACTORYCIRCUIT  Factory,
    _In_ const GUID *       UniqueId,
    _In_ WDFDEVICE          Device
    )
{
    NTSTATUS                status      = STATUS_UNSUCCESSFUL;
    PDSP_FACTORY_CONTEXT    factoryCtx  = GetDspFactoryContext(Factory);

    PAGED_CODE();

    WdfWaitLockAcquire(factoryCtx->CacheLock, NULL);

    //
    // Make sure there is not another device with the same ID.
    //
    if (Dsp_IsChildDeviceInCacheLocked(Factory, UniqueId))
    {
        status = STATUS_DEVICE_ALREADY_ATTACHED;
    }
    else
    {
        //
        // Attach a device ID context if not already present.
        //
        PDSP_DEVICEID_CONTEXT idCtx = GetDspDeviceIdContext(Device);
        if (idCtx == NULL)
        {
            // Add the device ID context.
            WDF_OBJECT_ATTRIBUTES   attributes;
            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSP_DEVICEID_CONTEXT);
            attributes.EvtCleanupCallback = Dsp_EvtDeviceIdContextCleanup;

            status = WdfObjectAllocateContext(Device, &attributes, (PVOID*)&idCtx);
            if (!NT_SUCCESS(status))
            {
                idCtx = NULL; // just in case.
                DrvLogError(g_SDCAVDspLog, FLAG_INIT, 
                            "Failed to allocate a DSP_DEVICEID_CONTEXT on WDFDEVICE %p, %!STATUS!",
                            Device, status);
            }
        }
        else
        {
            // This should not happen, but just in case, cleanup the context.
            ASSERT(FALSE);
            idCtx->UniqueID = NULL_GUID;
            if (idCtx->Factory != NULL)
            {
                WdfObjectDereferenceWithTag(idCtx->Factory, (PVOID)DRIVER_TAG);
                idCtx->Factory = NULL;
            }
        }

        if (idCtx != NULL)
        {
            //
            // Store the unique ID of this device.
            //
            idCtx->UniqueID = *UniqueId;

            //
            // Take a strong ref on the factory object.
            // Ref is removed on context cleanup.
            //
            idCtx->Factory = Factory;
            WdfObjectReferenceWithTag(Factory, (PVOID)DRIVER_TAG);

            //
            // Add the device to our cache.
            //
            status = WdfCollectionAdd(factoryCtx->Cache, Device);
        }
    }

    WdfWaitLockRelease(factoryCtx->CacheLock);

    return status;
}

PAGED_CODE_SEG
WDFDEVICE
Dsp_RemoveChildDeviceFromCache(
    _In_ ACXFACTORYCIRCUIT  Factory,
    _In_ const GUID *       UniqueId
    )
{
    PDSP_FACTORY_CONTEXT    factoryCtx  = GetDspFactoryContext(Factory);
    WDFDEVICE               child       = NULL;
    ULONG                   count;

    PAGED_CODE();

    WdfWaitLockAcquire(factoryCtx->CacheLock, NULL);

    count = WdfCollectionGetCount(factoryCtx->Cache);

    for (ULONG i = 0; i < count; i++)
    {
        PDSP_DEVICEID_CONTEXT   idCtx  = NULL;
        WDFDEVICE               device = NULL;

        device = (WDFDEVICE)WdfCollectionGetItem(factoryCtx->Cache, i);
        idCtx = GetDspDeviceIdContext(device);

        if ((idCtx != 0) && IsEqualGUID(idCtx->UniqueID, *UniqueId))
        {
            // Found it.
            // - zero out ID.
            // - add a ref for the caller.
            // - remove the item from the cache.
            idCtx->UniqueID = NULL_GUID;
            WdfObjectReferenceWithTag(device, (PVOID)DRIVER_TAG);
            WdfCollectionRemoveItem(factoryCtx->Cache, i);
            child = device;
            break;
        }
    }

    WdfWaitLockRelease(factoryCtx->CacheLock);

    return child;
}

PAGED_CODE_SEG
VOID
Dsp_PurgeChildDeviceFromCache(
    _In_ ACXFACTORYCIRCUIT  Factory,
    _In_ WDFDEVICE          Device
    )
{
    PDSP_FACTORY_CONTEXT    factoryCtx  = GetDspFactoryContext(Factory);
    PDSP_DEVICEID_CONTEXT   idCtx       = GetDspDeviceIdContext(Device);
    WDFDEVICE               child       = NULL;

    PAGED_CODE();

    WdfWaitLockAcquire(factoryCtx->CacheLock, NULL);

    //
    // Scan the cache only if the device's unique-id is not null.
    //
    if (idCtx != NULL && !IsEqualGUID(NULL_GUID, idCtx->UniqueID))
    {
        ULONG count = WdfCollectionGetCount(factoryCtx->Cache);

        for (ULONG i = 0; i < count; i++)
        {
            child = (WDFDEVICE)WdfCollectionGetItem(factoryCtx->Cache, i);
            if (child == Device)
            {
                //
                // Found it.
                // - zero out ID.
                // - remove the item from the cache.
                //
                idCtx->UniqueID = NULL_GUID;
                WdfCollectionRemoveItem(factoryCtx->Cache, i);
                break;
            }
        }
    }

    WdfWaitLockRelease(factoryCtx->CacheLock);
}

PAGED_CODE_SEG
VOID
Dsp_EvtDeviceIdContextCleanup(
    _In_ WDFOBJECT      Device
    )
{
    PDSP_DEVICEID_CONTEXT idCtx = GetDspDeviceIdContext(Device);

    PAGED_CODE();

    Dsp_PurgeChildDeviceFromCache(idCtx->Factory, (WDFDEVICE)Device);
    WdfObjectDereferenceWithTag(idCtx->Factory, (PVOID)DRIVER_TAG);
}

PAGED_CODE_SEG
VOID
Dsp_EvtFactoryCircuitRemoveCircuitCallback
(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
    )
{
    NTSTATUS                            status = STATUS_NOT_SUPPORTED;
    ACXFACTORYCIRCUIT                   factory = (ACXFACTORYCIRCUIT)Object;
    PDSP_FACTORY_CONTEXT                factoryCtx = GetDspFactoryContext(factory);
    WDFDEVICE                           child = NULL;
    PACX_FACTORY_CIRCUIT_REMOVE_CIRCUIT args;
    ULONG                               argsCb = sizeof(ACX_FACTORY_CIRCUIT_REMOVE_CIRCUIT);
    ACX_REQUEST_PARAMETERS              params;

    PAGED_CODE();

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    ASSERT(params.Type == AcxRequestTypeMethod);
    ASSERT(params.Parameters.Method.Verb == AcxMethodVerbSend);
    ASSERT(params.Parameters.Method.ArgsCb >= argsCb);
    
    args = (PACX_FACTORY_CIRCUIT_REMOVE_CIRCUIT)params.Parameters.Method.Args;
    argsCb = params.Parameters.Method.ArgsCb; // use real value.

    if (args->Size < argsCb)
    {
        status = STATUS_INVALID_PARAMETER;
         DrvLogError(g_SDCAVDspLog, FLAG_GENERIC,
            "ACX_FACTORY_CIRCUIT_REMOVE_CIRCUIT.Size %d is invalid, it should be >= %d, %!STATUS!",
            args->Size, argsCb, status);
        goto exit;
    }

    //
    // Remove the circut/circuit-device.
    // If found, there is a pending WDF ref on the object.
    //
    child = Dsp_RemoveChildDeviceFromCache(factory, &args->CircuitUniqueId);
    if (child == NULL)
    {
        // Device is gone. Nothing to do.
        status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Tell ACX not to enum this child device anymore.
    //
    status = AcxDeviceRemoveCircuitDevice(factoryCtx->Device, child);
    if (!NT_SUCCESS(status)) 
    {
         DrvLogError(g_SDCAVDspLog, FLAG_GENERIC,
                   "Parent %p, ACXFACTORYCIRCUIT %p, Child %p, AcxDeviceRemoveCircuitDevice failed, %!STATUS!",
                    factoryCtx->Device, factory, child, status);
        goto exit;
    }

    status = STATUS_SUCCESS;

exit:
    if (child != NULL)
    {
        WdfObjectDereferenceWithTag(child, (PVOID)DRIVER_TAG);
    }

    WdfRequestComplete(Request, status);
}

PAGED_CODE_SEG
VOID
Dsp_EvtFactoryRemoveCircuitRequestPreprocess(
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
    ASSERT(Object);
    ASSERT(Request);

    Dsp_EvtFactoryCircuitRemoveCircuitCallback(Object, Request);
}
#endif  //ACX_WORKAROUND_ACXFACTORYCIRCUIT_01


