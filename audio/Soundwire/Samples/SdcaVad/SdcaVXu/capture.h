/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    capture.h

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
DECLARE_CONST_UNICODE_STRING(CaptureDeviceId, L"SDCAVad\\ExtensionMicrophone");
DECLARE_CONST_UNICODE_STRING(CaptureHardwareId, L"SDCAVad\\ExtensionMicrophone");
DECLARE_CONST_UNICODE_STRING(CaptureInstanceId, L"00");
DECLARE_CONST_UNICODE_STRING(CaptureCompatibleId, SDCAVAD_COMPATIBLE_ID);
DECLARE_CONST_UNICODE_STRING(CaptureContainerId, SDCAVAD_CONTAINER_ID);
DECLARE_CONST_UNICODE_STRING(CaptureDeviceDescription, L"SDCAVad Microphone(Ext)");
DECLARE_CONST_UNICODE_STRING(CaptureDeviceLocation, L"SDCAVad Microphone");

PAGED_CODE_SEG
NTSTATUS
SdcaXuC_SetPowerPolicy(
    _In_ WDFDEVICE      Device
);

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateCaptureDevice(
    _In_ WDFDEVICE      Device,
    _Out_ WDFDEVICE     *CaptureDevice
);

PAGED_CODE_SEG
NTSTATUS
SdcaXu_AddDynamicCapture(
    _In_ WDFDEVICE      Device
);

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateCaptureCircuit(
    _In_    WDFDEVICE                   Device,
    _In_    PSDCAXU_ACX_CIRCUIT_CONFIG  CircuitConfig,
    _Out_   ACXCIRCUIT                  *Circuit
);

// Capture Device callbacks.

EVT_WDF_DEVICE_PREPARE_HARDWARE     SdcaXuC_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     SdcaXuC_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT SdcaXuC_EvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      SdcaXuC_EvtDeviceContextCleanup;

// Capture callbacks.

EVT_WDF_OBJECT_CONTEXT_CLEANUP      SdcaXuC_EvtCircuitContextCleanup;
EVT_ACX_OBJECT_PREPROCESS_REQUEST   SdcaXuC_EvtCircuitRequestPreprocess;
EVT_ACX_CIRCUIT_CREATE_STREAM       SdcaXuC_EvtCircuitCreateStream;
EVT_ACX_CIRCUIT_POWER_UP            SdcaXuC_EvtCircuitPowerUp;
EVT_ACX_CIRCUIT_POWER_DOWN          SdcaXuC_EvtCircuitPowerDown;
EVT_ACX_PIN_SET_DATAFORMAT          SdcaXuC_EvtAcxPinSetDataFormat;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      SdcaXuC_EvtPinContextCleanup;
EVT_ACX_OBJECT_PREPROCESS_REQUEST   SdcaXuC_EvtStreamRequestPreprocess;

#pragma code_seg()

