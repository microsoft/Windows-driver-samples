/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Windows User-Mode Driver Framework

--*/

#pragma once

class CQueue;

class CDevice
{
public:
    // This is called from PnP manager in response to AddDevice call
    static EVT_WDF_DRIVER_DEVICE_ADD OnDeviceAdd;

private:
    CDevice(_In_ WDFDEVICE Device);
    ~CDevice();

    static EVT_WDF_OBJECT_CONTEXT_CLEANUP OnCleanup;
    static EVT_WDF_DEVICE_D0_ENTRY OnD0Entry;
    static EVT_WDF_DEVICE_D0_EXIT OnD0Exit;

    NTSTATUS Initialize();
    CQueue* GetQueue();

    WDFDEVICE _Device = nullptr;
    CQueue* _Queue = nullptr;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CDevice, GetDeviceObject);
