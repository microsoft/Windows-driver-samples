// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "netvadapter.h"

class NetvDevice
{

public:

    //
    // Do not add variable before WdfTriageInfoPtr.
    // NetAdapterCx carving code requires the first field of
    // WDF context to be a pointer to WDF_TRIAGE_INFO.
    //
    void *
        m_triage = nullptr;

    NetvDevice(
        WDFDEVICE Handle
    );

    NTSTATUS
    Initialize(
        void
    );

    NTSTATUS
    PrepareHardware(
        WDFCMRESLIST ResourcesRaw,
        WDFCMRESLIST ResourcesTranslated
    );

public: // private:

    WDFDEVICE
        m_handle = WDF_NO_HANDLE;

    NetvAdapter *
        m_adapter = nullptr;

};
static_assert(FIELD_OFFSET(NetvDevice, m_triage) == 0u);

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NetvDevice, NetvDeviceGetContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NetvAdapter, NetvAdapterGetContext);

EVT_WDF_DEVICE_PREPARE_HARDWARE
    EvtDevicePrepareHardware;
