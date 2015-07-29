/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Device.h

Abstract:

    This module contains the type definitions for the
    driver's device callback class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/
#pragma once

class CQueue;

class CDevice
{
public:
    CDevice(WDFDEVICE Device) : m_Device(Device), m_pQueue(nullptr)
    {}

public:
    static EVT_WDF_DRIVER_DEVICE_ADD OnDeviceAdd;
    static EVT_WDF_OBJECT_CONTEXT_CLEANUP OnCleanup;
    static EVT_WDF_DEVICE_D0_ENTRY OnD0Entry;
    static EVT_WDF_DEVICE_D0_EXIT OnD0Exit;
    static EVT_WDF_DEVICE_FILE_CREATE OnFileCreate;
    static EVT_WDF_FILE_CLOSE OnFileClose;

public:
    NTSTATUS Initialize();
    NTSTATUS Deinitialize();

    NTSTATUS OnD0Entry(_In_ WDF_POWER_DEVICE_STATE PreviousState);
    NTSTATUS OnD0Exit(_In_ WDF_POWER_DEVICE_STATE TargetState);

public:
    CQueue* GetQueue()
    {
        return m_pQueue;
    }

private:
    WDFDEVICE   m_Device;
    CQueue*     m_pQueue;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CDevice, GetDeviceObject);