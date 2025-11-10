// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

//
// SharedTypes.h
// Central header for global definitions, macros, and shared structures.
//
#include "precomp.h"
#include <initguid.h> // for GUID defination

// =============================
// GUIDs or Constants
// =============================
// {bb67559a-06f6-4eb0-81e9-21fdc3b60efb}
DEFINE_GUID(GUID_WIFICX_SAMPLE_CLIENT_INTERFACE, 0xbb67559a, 0x06f6, 0x4eb0, 0x81, 0xe9, 0x21, 0xfd, 0xc3, 0xb6, 0x0e, 0xfb);
#define WIFI_DRIVER_DEFAULT_POOL_TAG 'sHIW' // WIFI IHV Sample Driver

// =============================
// WDF Object Context Types
// =============================

typedef struct _WIFI_DEVICE_CONTEXT
{
    //
    // Do not add field variable before WdfTriageInfoPtr.
    // NetAdapterCx carving code requires the first field of WDF context
    // to be a pointer to WDF_TRIAGE_INFO.
    //
    void* WdfTriageInfoPtr;
    WDFDEVICE WdfDevice;
    UCHAR CurrentRadioState;
    WDI_GET_ADAPTER_CAPABILITIES_PARAMETERS AdapterCapabilities;
    TLV_CONTEXT TlvContext;
    UINT32 LastConnectEntryId;
    UINT32 LastConnectTransactionId;
    WDI_AUTH_ALGORITHM LastAuthAlgo;
    DOT11_MAC_ADDRESS ConnectedPeer;
    NETADAPTER primaryStaAdapter;
} WIFI_DEVICE_CONTEXT, * PWIFI_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WIFI_DEVICE_CONTEXT, WifiGetDeviceContext);
static_assert(FIELD_OFFSET(WIFI_DEVICE_CONTEXT, WdfTriageInfoPtr) == 0);

// Context for each NetAdapter instance.
// Each NetAdapter instance corresponds to an IP interface
//
typedef struct _WIFI_NETADAPTER_CONTEXT
{
    PWIFI_DEVICE_CONTEXT WifiDeviceContext; // Wdf device context
    NETADAPTER NetAdapter;                  // NetAdapter object

    LIST_ENTRY ReceiveList;
    NETPACKETQUEUE TxQueue; // Tx Queue object
    NETPACKETQUEUE RxQueue; // Rx Queue object
    bool CanReportWifiWakeSourceTypeClientDriverDiagnostic;
} WIFI_NETADAPTER_CONTEXT, * PWIFI_NETADAPTER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WIFI_NETADAPTER_CONTEXT, WifiGetNetAdapterContext);

typedef struct _PLACEMENT_NEW_ALLOCATION_CONTEXT
{
    size_t cbMaxSize;
    _Field_size_bytes_(cbMaxSize) void* pbBuffer;
} PLACEMENT_NEW_ALLOCATION_CONTEXT, * PPLACEMENT_NEW_ALLOCATION_CONTEXT;
typedef const PLACEMENT_NEW_ALLOCATION_CONTEXT* PCPLACEMENT_NEW_ALLOCATION_CONTEXT;

// for FreeWdfMemoryBuffer to correct get
// the handle to free the memory
struct WIFI_IHV_MEMORY_HEADER
{
    size_t HeaderSize;
    WDFMEMORY WdfMemoryHandle;
};

// for tracking memory leaks
struct WIFI_IHV_MEMORY_CONTEXT
{
    size_t ContextSize;
    void* pvCaller;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WIFI_IHV_MEMORY_CONTEXT, GetWificxIhvMemoryContextFromHandle);
