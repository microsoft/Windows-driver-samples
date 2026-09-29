/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    render.h

Abstract:

    Contains structure definitions and function prototypes private to
    the driver.

Environment:

    Kernel mode

--*/

#pragma once

//
// Circuit's settings for raw PDO.
//
DECLARE_CONST_UNICODE_STRING(RenderDeviceId, L"SDCAVad\\ExtensionSpeaker");
DECLARE_CONST_UNICODE_STRING(RenderHardwareId, L"SDCAVad\\ExtensionSpeaker");
DECLARE_CONST_UNICODE_STRING(RenderInstanceId, L"00");
DECLARE_CONST_UNICODE_STRING(RenderCompatibleId, SDCAVAD_COMPATIBLE_ID);
DECLARE_CONST_UNICODE_STRING(RenderContainerId, SDCAVAD_CONTAINER_ID);
DECLARE_CONST_UNICODE_STRING(RenderDeviceDescription, L"SDCAVad Speaker(Ext)");
DECLARE_CONST_UNICODE_STRING(RenderDeviceLocation, L"SDCAVad Speaker");

PAGED_CODE_SEG
NTSTATUS
SdcaXuR_SetPowerPolicy(
    _In_ WDFDEVICE      Device
);

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateRenderDevice(
    _In_ WDFDEVICE      Device,
    _Out_ WDFDEVICE     *RenderDevice
);

PAGED_CODE_SEG
NTSTATUS
SdcaXu_AddDynamicRender(
    _In_ WDFDEVICE      Device
);

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateRenderCircuit(
    _In_ WDFDEVICE                      Device,
    _In_ PSDCAXU_ACX_CIRCUIT_CONFIG     CircuitConfig,
    _Out_ ACXCIRCUIT                    *Circuit
);

// Render Device callbacks.

EVT_WDF_DEVICE_PREPARE_HARDWARE     SdcaXuR_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     SdcaXuR_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT SdcaXuR_EvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      SdcaXuR_EvtDeviceContextCleanup;

// Render callbacks.

EVT_WDF_OBJECT_CONTEXT_CLEANUP      SdcaXuR_EvtCircuitContextCleanup;
EVT_ACX_OBJECT_PREPROCESS_REQUEST   SdcaXuR_EvtCircuitRequestPreprocess;
EVT_ACX_CIRCUIT_CREATE_STREAM       SdcaXuR_EvtCircuitCreateStream;
EVT_ACX_CIRCUIT_POWER_UP            SdcaXuR_EvtCircuitPowerUp;
EVT_ACX_CIRCUIT_POWER_DOWN          SdcaXuR_EvtCircuitPowerDown;
EVT_ACX_PIN_SET_DATAFORMAT          SdcaXuR_EvtAcxPinSetDataFormat;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      SdcaXuR_EvtPinContextCleanup;
EVT_ACX_OBJECT_PREPROCESS_REQUEST   SdcaXuR_EvtStreamRequestPreprocess;

#pragma code_seg()

