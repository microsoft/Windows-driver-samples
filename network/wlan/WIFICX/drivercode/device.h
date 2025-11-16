// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once
#include "wifiHAL.h"
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtDeviceReleaseHardware;

EVT_WIFI_DEVICE_CREATE_ADAPTER EvtWifiDeviceCreateAdapter;
EVT_WIFI_DEVICE_CREATE_WIFIDIRECTDEVICE EvtWifiDeviceCreateWifiDirectDevice;
EVT_WIFI_DEVICE_SEND_COMMAND EvtWifiDeviceSendCommand;
EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtAdapterCleanup;

typedef struct _WIFI_IHV_DEVICE_CONTEXT
{
    //
    // Do not add field variable before WdfTriageInfoPtr.
    // NetAdapterCx carving code requires the first field of WDF context
    // to be a pointer to WDF_TRIAGE_INFO.
    //
    void* WdfTriageInfoPtr;
    WDFDEVICE WdfDevice;
    TLV_CONTEXT TlvContext;
    NETADAPTER primaryStaAdapter;
    WifiHAL* wifiHAL;
} WIFI_IHV_DEVICE_CONTEXT, * PWIFI_IHV_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WIFI_IHV_DEVICE_CONTEXT, WifiGetIhvDeviceContext);
static_assert(FIELD_OFFSET(WIFI_IHV_DEVICE_CONTEXT, WdfTriageInfoPtr) == 0);
