/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CircuitDevice.h

Abstract:

    Raw PDO for ACX circuits. This file contains routines to create Device
    and handle pnp requests

Environment:

    Kernel mode

--*/

#pragma once

//
// Circuit's settings for raw PDO.
//
DECLARE_CONST_UNICODE_STRING(CircuitDeviceId, L"SDCAVad\\ExtensionDevice");
DECLARE_CONST_UNICODE_STRING(CircuitHardwareId, L"SDCAVad\\ExtensionDevice");
DECLARE_CONST_UNICODE_STRING(CircuitInstanceId, L"00");
DECLARE_CONST_UNICODE_STRING(CircuitCompatibleId, SDCAVAD_COMPATIBLE_ID);
DECLARE_CONST_UNICODE_STRING(CircuitContainerId, SDCAVAD_CONTAINER_ID);
DECLARE_CONST_UNICODE_STRING(CircuitDeviceDescription, L"SDCAVad Device (Ext)");
DECLARE_CONST_UNICODE_STRING(CircuitDeviceLocation, L"SDCAVad Device");

PAGED_CODE_SEG
NTSTATUS
SDCAVXu_CreateCircuitDevice(
    _In_ WDFDEVICE Device
);

PAGED_CODE_SEG
NTSTATUS
SdcaXuCircuit_SetPowerPolicy(
    _In_ WDFDEVICE      Device
);

// Render Device callbacks.
EVT_WDF_DEVICE_PREPARE_HARDWARE     SdcaXuCircuit_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     SdcaXuCircuit_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT SdcaXuCircuit_EvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      SdcaXuCircuit_EvtDeviceContextCleanup;

#pragma code_seg()

