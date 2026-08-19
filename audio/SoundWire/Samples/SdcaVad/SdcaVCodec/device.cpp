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
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>
#include "streamengine.h"
#include "CircuitHelper.h"

#ifndef __INTELLISENSE__
#include "device.tmh"
#endif

UNICODE_STRING g_RegistryPath = {0};      // This is used to store the registry settings path for the driver

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS CopyRegistrySettingsPath
(
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
Codec_EvtBusDeviceAdd(
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

    //
    // Allow ACX to add any pre-requirement it needs on this device.
    //
    ACX_DEVICEINIT_CONFIG devInitCfg;
    ACX_DEVICEINIT_CONFIG_INIT(&devInitCfg);
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitInitialize(DeviceInit, &devInitCfg));
    
    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = Codec_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = Codec_EvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Specify the type of context needed. 
    // Use default locking, i.e., none.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = Codec_EvtDeviceContextCleanup;
    
    //
    // Create the device.
    //
    WDFDEVICE device = NULL;
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&DeviceInit, &attributes, &device));

    //
    // Init Codec's device context.
    //
    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext(device);
    ASSERT(devCtx != NULL);
    devCtx->Render = NULL;
    devCtx->Capture = NULL;

    //
    // Assume XU lower filter driver is not present
    //
    devCtx->SdcaXuData.bSdcaXu = FALSE;
    
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
    // Get SDCA XU filter interface
    //
    RETURN_NTSTATUS_IF_FAILED_UNLESS_ALLOWED(Codec_GetSdcaXu(device), STATUS_NOT_SUPPORTED);

    if (devCtx->SdcaXuData.bSdcaXu)
    {
        RETURN_NTSTATUS_IF_FAILED(Codec_SetSdcaXuHwConfig(device));
    }

    RETURN_NTSTATUS_IF_FAILED(Codec_AddRenderComposites(device));

    RETURN_NTSTATUS_IF_FAILED(Codec_AddCaptureComposites(device));

    //
    // Add a render device and a capture device. 
    //
    RETURN_NTSTATUS_IF_FAILED(CodecR_AddRenders(Driver, device));

    //
    // Add a render device and a capture device. 
    //
    RETURN_NTSTATUS_IF_FAILED(CodecC_AddCaptures(Driver, device));

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Codec_EvtDevicePrepareHardware(
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
    
    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    DrvLogEnter(g_SDCAVCodecLog);

    NTSTATUS status = STATUS_SUCCESS;
    
    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext(Device);
    ASSERT(devCtx != NULL);


    RETURN_NTSTATUS_IF_FAILED(Codec_SetPowerPolicy(Device));
    
    //
    // Add static circuit to device's list.
    //
    ASSERT(devCtx->Render);
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddCircuit(Device, devCtx->Render));

    ASSERT(devCtx->Capture);
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddCircuit(Device, devCtx->Capture));
    
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Codec_EvtDeviceReleaseHardware(
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

    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext(Device);
    ASSERT(devCtx != NULL);

    
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Codec_SetPowerPolicy(
    _In_ WDFDEVICE      Device
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVCodecLog);

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    //WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;

    //
    // Init the idle policy structure.
    //
    //WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCanWakeFromS0);
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeout = 10000; // 10-sec

    RETURN_NTSTATUS_IF_FAILED(WdfDeviceAssignS0IdleSettings(Device, &idleSettings));

    return status;
}

#pragma code_seg()

DEFINE_GUID(CODEC_CIRCUIT_RENDER_GUID, 
0xfd4b6e78, 0x51e0, 0x4aa6, 0x90, 0x98, 0xbb, 0xcb, 0x70, 0x89, 0xcb, 0x6a);

DEFINE_GUID(EXTENSION_CIRCUIT_RENDER_GUID, 
0x656ab905, 0x55fb, 0x4b08, 0xb6, 0x01, 0xd7, 0xf0, 0xc1, 0xce, 0x36, 0x2c);

// {17F5B19F-C2C7-4B53-AFB9-49A0283D0DCE}
DEFINE_GUID(DSP_CIRCUIT_SPEAKER_GUID,
    0x17f5b19f, 0xc2c7, 0x4b53, 0xaf, 0xb9, 0x49, 0xa0, 0x28, 0x3d, 0xd, 0xce);

DEFINE_GUID(CODEC_CIRCUIT_CAPTURE_GUID,
0x67ec5936, 0xa395, 0x4e93, 0xbe, 0x8a, 0xfc, 0xed, 0xe3, 0x1b, 0xad, 0x40);

DEFINE_GUID(EXTENSION_CIRCUIT_CAPTURE_GUID,
0x44c69385, 0xa012, 0x405f, 0x8a, 0x9a, 0x7b, 0x44, 0x29, 0x71, 0xc8, 0x50);

// {6F9EACF7-CD2D-4030-9E49-7CC4ADEFF192}
DEFINE_GUID(DSP_CIRCUIT_MICROPHONE_GUID,
    0x6f9eacf7, 0xcd2d, 0x4030, 0x9e, 0x49, 0x7c, 0xc4, 0xad, 0xef, 0xf1, 0x92);

DEFINE_GUID(SYSTEM_CONTAINER_GUID,
0x00000000, 0x0000, 0x0000, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);


#define DSP_FACTORY_URI L"acpi:obj-path:\\_SB.PC00.HDAS"

#define RENDER_CIRCUIT_UNIQUE_ID L"{613fd364-64bb-4b69-99bc-5b075ea9756b}"
#define RENDER_CIRCUIT_FRIENDLY_NAME L"Speaker-360"
#define RENDER_CIRCUIT_NAME L"Speaker"
#define CAPTURE_CIRCUIT_UNIQUE_ID L"{3A509246-5902-4AA2-9E06-C7C8D10461C3}"
#define CAPTURE_CIRCUIT_FRIENDLY_NAME L"Microphone-360"
#define CAPTURE_CIRCUIT_NAME L"Microphone"

#define CIRCUIT_RENDER_VENDOR_BLOB "Streaming_Speaker"
#define CIRCUIT_CAPTURE_VENDOR_BLOB "Streaming_MicrophoneArray"

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS 
Codec_AddRenderComposites(_In_ WDFDEVICE Device)
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Device);

    PAGED_CODE();

    RETURN_NTSTATUS_IF_FAILED(Codec_AddComposites(Device, CompositeType_RENDER));

    return status;
}

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS 
Codec_AddCaptureComposites(_In_ WDFDEVICE Device)
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Device);

    PAGED_CODE();

    RETURN_NTSTATUS_IF_FAILED(Codec_AddComposites(Device, CompositeType_CAPTURE));

    return status;
}

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS 
Codec_AddComposites(_In_ WDFDEVICE Device, _In_ CompositeType compositeType)
{
    PAGED_CODE(); 

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVCodecLog);

    UNICODE_STRING circuit_IDs[] = {
        { sizeof(RENDER_CIRCUIT_UNIQUE_ID) - sizeof(WCHAR), sizeof(RENDER_CIRCUIT_UNIQUE_ID), RENDER_CIRCUIT_UNIQUE_ID },
        { sizeof(CAPTURE_CIRCUIT_UNIQUE_ID) - sizeof(WCHAR), sizeof(CAPTURE_CIRCUIT_UNIQUE_ID), CAPTURE_CIRCUIT_UNIQUE_ID}
    };

    UNICODE_STRING circuit_friendly_names[] = {
        { sizeof(RENDER_CIRCUIT_FRIENDLY_NAME) - sizeof(WCHAR), sizeof(RENDER_CIRCUIT_FRIENDLY_NAME), RENDER_CIRCUIT_FRIENDLY_NAME },
        { sizeof(CAPTURE_CIRCUIT_FRIENDLY_NAME) - sizeof(WCHAR), sizeof(CAPTURE_CIRCUIT_FRIENDLY_NAME), CAPTURE_CIRCUIT_FRIENDLY_NAME}
    };

    UNICODE_STRING circuit_names[] = {
        { sizeof(RENDER_CIRCUIT_NAME) - sizeof(WCHAR), sizeof(RENDER_CIRCUIT_NAME), RENDER_CIRCUIT_NAME },
        { sizeof(CAPTURE_CIRCUIT_NAME) - sizeof(WCHAR), sizeof(CAPTURE_CIRCUIT_NAME), CAPTURE_CIRCUIT_NAME}
    };

    UNICODE_STRING codec_circuit_uris[] = {
        { sizeof(RENDER_CIRCUIT_URI) - sizeof(WCHAR), sizeof(RENDER_CIRCUIT_URI), RENDER_CIRCUIT_URI },
        { sizeof(CAPTURE_CIRCUIT_URI) - sizeof(WCHAR), sizeof(CAPTURE_CIRCUIT_URI), CAPTURE_CIRCUIT_URI}
    };

    UNICODE_STRING extension_circuit_uris[] = {
        { sizeof(EXT_RENDER_CIRCUIT_URI) - sizeof(WCHAR), sizeof(EXT_RENDER_CIRCUIT_URI), EXT_RENDER_CIRCUIT_URI },
        { sizeof(EXT_CAPTURE_CIRCUIT_URI) - sizeof(WCHAR), sizeof(EXT_CAPTURE_CIRCUIT_URI), EXT_CAPTURE_CIRCUIT_URI}
    };

    GUID dsp_circuit_guids[] = {
        DSP_CIRCUIT_SPEAKER_GUID,
        DSP_CIRCUIT_MICROPHONE_GUID
    };

    UNICODE_STRING dsp_factory_uris[] = {
        { sizeof(DSP_FACTORY_URI) - sizeof(WCHAR), sizeof(DSP_FACTORY_URI), DSP_FACTORY_URI },
        { sizeof(DSP_FACTORY_URI) - sizeof(WCHAR), sizeof(DSP_FACTORY_URI), DSP_FACTORY_URI}
    };

    const char* dsp_factory_vendor_blobs[] = {
        CIRCUIT_RENDER_VENDOR_BLOB,
        CIRCUIT_CAPTURE_VENDOR_BLOB
    };

    PCODEC_DEVICE_CONTEXT deviceCtx = NULL;
    deviceCtx = GetCodecDeviceContext(Device);
    ASSERT(deviceCtx);

    //
    // May be called again for rebalance
    // Add composites only once
    //
    RETURN_NTSTATUS_IF_TRUE(0 != deviceCtx->refComposite[compositeType], STATUS_SUCCESS);

    //
    // Object bag
    //
    // This obj-bag config setting is shared by all composite/circuit templates.
    ACX_OBJECTBAG_CONFIG objBagCfg;
    ACX_OBJECTBAG_CONFIG_INIT(&objBagCfg);

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = AcxGetManager(NULL);

    ACXOBJECTBAG objBag = NULL;
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagCreate(&attributes, &objBagCfg, &objBag));
    auto objBag_scope = scope_exit([&objBag]() {
        if (objBag != NULL)
        {
            WdfObjectDelete(objBag);
        }
    });

    //
    // Add a test unsigned int 4 bytes to the object bag
    //
    RETURN_NTSTATUS_IF_FAILED(ObjBagAddTestUI4(objBag, 0));

    //
    // Add unique circuit ID to the object bag
    // This unique Id will be picked up by DSP circuit
    //
    DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(UniqueID);
    GUID uniqueID = { 0 };
    RETURN_NTSTATUS_IF_FAILED(RtlGUIDFromString(&circuit_IDs[compositeType], &uniqueID));

    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddGuid(objBag, &UniqueID, uniqueID));

    RETURN_NTSTATUS_IF_FAILED(ObjBagAddUnicodeStrings(objBag, circuit_friendly_names[compositeType], circuit_names[compositeType]));

    RETURN_NTSTATUS_IF_FAILED(ObjBagAddEndpointId(objBag, 9));

    RETURN_NTSTATUS_IF_FAILED(ObjBagAddDataPortNumber(objBag, 9));

    //
    // Composite template.
    //
    ULONG circuitsInTemplate = 0;
    ACXCIRCUITTEMPLATE circuits[3] = { 0 };
    ACX_COMPOSITE_TEMPLATE_CONFIG compositeCfg;
    ACX_COMPOSITE_TEMPLATE_CONFIG_INIT(&compositeCfg);
    compositeCfg.Properties = objBag;
    compositeCfg.Flags |= AcxCompositeTemplateConfigSingleton;

    ACXCOMPOSITETEMPLATE composite = NULL;
    RETURN_NTSTATUS_IF_FAILED(AcxCompositeTemplateCreate(WdfGetDriver(),
        &attributes,
        &compositeCfg,
        &composite));

    auto composite_scope = scope_exit([&composite]() {
        WdfObjectDelete(composite);
        composite = NULL;
        });

    objBag = NULL;

    // This attribute setting is shared by all the circuit templates.
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = composite;

    // Codec template.
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagCreate(&attributes, &objBagCfg, &objBag));

    RETURN_NTSTATUS_IF_FAILED(ObjBagAddTestUI4(objBag, 2));

    ACX_CIRCUIT_TEMPLATE_CONFIG circuitCfg1;
    ACX_CIRCUIT_TEMPLATE_CONFIG_INIT(&circuitCfg1);
    circuitCfg1.CircuitProperties = objBag;
    circuitCfg1.CircuitUri = &codec_circuit_uris[compositeType];

    ULONG codecIndex = circuitsInTemplate;
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitTemplateCreate(WdfGetDriver(),
        &attributes,
        &circuitCfg1,
        &circuits[circuitsInTemplate++]));

    objBag = NULL;

    // XU template.
    //
    // Check if XU is present
    // and compose with Xu circuit
    //
    if (deviceCtx->SdcaXuData.bSdcaXu)
    {
        RETURN_NTSTATUS_IF_FAILED(AcxObjectBagCreate(&attributes, &objBagCfg, &objBag));

        RETURN_NTSTATUS_IF_FAILED(ObjBagAddTestUI4(objBag, 2));

        ACX_CIRCUIT_TEMPLATE_CONFIG circuitCfg2;
        ACX_CIRCUIT_TEMPLATE_CONFIG_INIT(&circuitCfg2);
        circuitCfg2.CircuitProperties = objBag;
        circuitCfg2.CircuitUri = &extension_circuit_uris[compositeType];

        RETURN_NTSTATUS_IF_FAILED(AcxCircuitTemplateCreate(WdfGetDriver(),
            &attributes,
            &circuitCfg2,
            &circuits[circuitsInTemplate++]));

        objBag = NULL;
    }

    // Dsp template.
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagCreate(&attributes, &objBagCfg, &objBag));

    RETURN_NTSTATUS_IF_FAILED(ObjBagAddTestUI4(objBag, 3));

    RETURN_NTSTATUS_IF_FAILED(ObjBagAddCircuitId(objBag, dsp_circuit_guids[compositeType]));

    RETURN_NTSTATUS_IF_FAILED(ObjBagAddBlob(objBag, dsp_factory_vendor_blobs[compositeType]));

    ACX_CIRCUIT_TEMPLATE_CONFIG circuitCfg3;
    ACX_CIRCUIT_TEMPLATE_CONFIG_INIT(&circuitCfg3);
    circuitCfg3.CircuitProperties = objBag;
    circuitCfg3.FactoryUri = &dsp_factory_uris[compositeType];
    circuitCfg3.Flags |= AcxCircuitTemplateCircuitOnDemand;

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitTemplateCreate(WdfGetDriver(),
        &attributes,
        &circuitCfg3,
        &circuits[circuitsInTemplate++]));

    objBag = NULL;
    objBag_scope.release();

    RETURN_NTSTATUS_IF_FAILED(AcxCompositeTemplateAssignCircuits(composite, circuits, circuitsInTemplate));

    // Select the core circuit.
    AcxCompositeTemplateSetCoreCircuit(composite, circuits[codecIndex]);

    // Final step.                                              
    RETURN_NTSTATUS_IF_FAILED(AcxManagerAddCompositeTemplate(AcxGetManager(NULL), composite));

    deviceCtx->Composite[compositeType] = composite;
    composite_scope.release();

    deviceCtx->refComposite[compositeType]++;
    
    return status;
}

#pragma code_seg()
NTSTATUS 
Codec_RemoveComposites(_In_ WDFDEVICE Device)
{
    NTSTATUS status = STATUS_SUCCESS;

    PCODEC_DEVICE_CONTEXT deviceCtx = NULL;
    deviceCtx = GetCodecDeviceContext(Device);
    ASSERT(deviceCtx);

    for (ULONG compositeType = CompositeType_RENDER; compositeType <= CompositeType_CAPTURE; )
    {
        if (deviceCtx->refComposite[compositeType])
        {
            deviceCtx->refComposite[compositeType]--;
            if (deviceCtx->refComposite[compositeType] == 0)
            {
                if (deviceCtx->Composite[compositeType] != NULL)
                {
                    RETURN_NTSTATUS_IF_FAILED(AcxManagerRemoveCompositeTemplate(AcxGetManager(NULL), deviceCtx->Composite[compositeType]));

                    WdfObjectDelete(deviceCtx->Composite[compositeType]);
                    deviceCtx->Composite[compositeType] = NULL;
                }
            }
        }

        compositeType++;
    }
    
    return status;
}

#pragma code_seg()
VOID
Codec_EvtDeviceContextCleanup(
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
    PCODEC_DEVICE_CONTEXT   devCtx;


    device = (WDFDEVICE)WdfDevice;
    devCtx = GetCodecDeviceContext(device);
    ASSERT(devCtx != NULL);
    
    Codec_RemoveComposites(device);

    if (devCtx->SdcaXuData.XUEntities)
    {
        ExFreePoolWithTag(devCtx->SdcaXuData.XUEntities, DRIVER_TAG);
        devCtx->SdcaXuData.numXUEntities = 0;
    }
    if (devCtx->SdcaXuData.InterruptInfo)
    {
        ExFreePoolWithTag(devCtx->SdcaXuData.InterruptInfo, DRIVER_TAG);
        devCtx->SdcaXuData.InterruptInfo = NULL;
    }
}

#pragma code_seg()
VOID
Codec_EvtStreamDestroy(
    _In_ WDFOBJECT Object
    )
{
    PCODEC_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    ctx = GetCodecStreamContext((ACXSTREAM)Object);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;
    ctx->StreamEngine = NULL;
    delete streamEngine;
}

PAGED_CODE_SEG
NTSTATUS
Codec_EvtStreamGetHwLatency(
    _In_ ACXSTREAM Stream,
    _Out_ ULONG * FifoSize,
    _Out_ ULONG * Delay
)
{
    PCODEC_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetCodecStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->GetHWLatency(FifoSize, Delay);
}

PAGED_CODE_SEG
NTSTATUS
Codec_EvtStreamPrepareHardware(
    _In_ ACXSTREAM Stream
    )
{
    PCODEC_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetCodecStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->PrepareHardware();
}

PAGED_CODE_SEG
NTSTATUS
Codec_EvtStreamReleaseHardware(
    _In_ ACXSTREAM Stream
    )
{
    PCODEC_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetCodecStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->ReleaseHardware();
}

PAGED_CODE_SEG
NTSTATUS
Codec_EvtStreamRun(
    _In_ ACXSTREAM Stream
    )
{
    PCODEC_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetCodecStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->Run();
}


PAGED_CODE_SEG
NTSTATUS
Codec_EvtStreamPause(
    _In_ ACXSTREAM Stream
    )
{
    PCODEC_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetCodecStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->Pause();
}

PAGED_CODE_SEG
NTSTATUS
Codec_EvtStreamAssignDrmContentId(
    _In_ ACXSTREAM      Stream,
    _In_ ULONG          DrmContentId,
    _In_ PACXDRMRIGHTS  DrmRights
    )
{
    PCODEC_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetCodecStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->AssignDrmContentId(DrmContentId, DrmRights);
}


