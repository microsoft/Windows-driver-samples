/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    wmi.c

Abstract:

    This modules contains the Windows Driver Framework WMI
    handlers for the firefly filter driver.

Environment:

    Kernel mode

--*/

#include "FireFly.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, WmiInitialize)
#pragma alloc_text(PAGE, EvtWmiInstanceQueryInstance)
#pragma alloc_text(PAGE, EvtWmiInstanceSetInstance)
#pragma alloc_text(PAGE, EvtWmiInstanceSetItem)
#endif

//
// Register our GUID and Datablock generated from the Firefly.mof file.
//
NTSTATUS
WmiInitialize(
    WDFDEVICE       Device,
    PDEVICE_CONTEXT DeviceContext
    )
{
    WDF_WMI_PROVIDER_CONFIG providerConfig;
    WDF_WMI_INSTANCE_CONFIG instanceConfig;
    WDF_OBJECT_ATTRIBUTES woa;
    WDFWMIINSTANCE instance;
    NTSTATUS status;
    DECLARE_CONST_UNICODE_STRING(mofRsrcName, MOFRESOURCENAME);

    UNREFERENCED_PARAMETER(DeviceContext);

    PAGED_CODE();

    status = WdfDeviceAssignMofResourceName(Device, &mofRsrcName);
    if (!NT_SUCCESS(status)) {
        KdPrint(("FireFly: Error in WdfDeviceAssignMofResourceName %x\n", status));
        return status;
    }

    WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &FireflyDeviceInformation_GUID);
    providerConfig.MinInstanceBufferSize = sizeof(FireflyDeviceInformation);

    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);
    instanceConfig.Register = TRUE;
    instanceConfig.EvtWmiInstanceQueryInstance = EvtWmiInstanceQueryInstance;
    instanceConfig.EvtWmiInstanceSetInstance = EvtWmiInstanceSetInstance;
    instanceConfig.EvtWmiInstanceSetItem = EvtWmiInstanceSetItem;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&woa, FireflyDeviceInformation);

    //
    // No need to store the WDFWMIINSTANCE in the device context because it is
    // passed back in the WMI instance callbacks and is not referenced outside
    // of those callbacks.
    //
    status = WdfWmiInstanceCreate(Device, &instanceConfig, &woa, &instance);

    if (NT_SUCCESS(status)) {
        FireflyDeviceInformation* info;

        info = InstanceGetInfo(instance);
        info->TailLit = TRUE;
    }

    return status;
}

NTSTATUS
EvtWmiInstanceQueryInstance(
    IN  WDFWMIINSTANCE WmiInstance,
    IN  ULONG OutBufferSize,
    IN  PVOID OutBuffer,
    OUT PULONG BufferUsed
    )
{
    FireflyDeviceInformation* pInfo;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(OutBufferSize);

    pInfo = InstanceGetInfo(WmiInstance);

    //
    // Our mininum buffer size has been checked by the Framework
    // and failed automatically if too small.
    //
    *BufferUsed = sizeof(*pInfo);

    RtlCopyMemory(OutBuffer, pInfo, sizeof(*pInfo));

    return STATUS_SUCCESS;
}

NTSTATUS
EvtWmiInstanceSetInstance(
    IN  WDFWMIINSTANCE WmiInstance,
    IN  ULONG InBufferSize,
    IN  PVOID InBuffer
    )
{
    FireflyDeviceInformation* pInfo;
    ULONG length;
    NTSTATUS status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(InBufferSize);

    pInfo = InstanceGetInfo(WmiInstance);

    //
    // Our mininum buffer size has been checked by the Framework
    // and failed automatically if too small.
    //
    length = sizeof(*pInfo);

    RtlMoveMemory(pInfo, InBuffer, length);

    //
    // Tell the HID device about the new tail light state
    //
    status = FireflySetFeature(
        WdfObjectGet_DEVICE_CONTEXT(WdfWmiInstanceGetDevice(WmiInstance)),
        TAILLIGHT_PAGE,
        TAILLIGHT_FEATURE,
        pInfo->TailLit
        );

    return status;
}

NTSTATUS
EvtWmiInstanceSetItem(
    IN  WDFWMIINSTANCE WmiInstance,
    IN  ULONG DataItemId,
    IN  ULONG InBufferSize,
    IN  PVOID InBuffer
    )
{
    NTSTATUS status;
    FireflyDeviceInformation* pInfo;

    PAGED_CODE();

    pInfo = InstanceGetInfo(WmiInstance);

    if (DataItemId == 1) {
        if (InBufferSize < FireflyDeviceInformation_TailLit_SIZE) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        pInfo->TailLit = (*(PBOOLEAN) InBuffer) ? TRUE : FALSE;

        //
        // Tell the HID device about the new tail light state
        //
        status = FireflySetFeature(
            WdfObjectGet_DEVICE_CONTEXT(WdfWmiInstanceGetDevice(WmiInstance)),
            TAILLIGHT_PAGE,
            TAILLIGHT_FEATURE,
            pInfo->TailLit
            );

        return status;
    }
    else {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

