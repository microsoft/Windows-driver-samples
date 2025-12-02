//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once

typedef struct _WMBCLASS_DEVICE_CONTEXT
{
    void* WdfTriageInfoPtr;

    WDFDEVICE WdfDevice;
    bool m_armedWake;
    ULONG TraceInstance;

    MBB_BUS_HANDLE BusHandle;
    MBB_BUS_PARAMETERS BusParams;

    ULONGLONG DSSPacketsReceivedCount;
    ULONGLONG DSSPacketsSentCount;

    LONG NtbSequenceNumber;

    WDFLOOKASIDE ReceiveLookasideList;
    PUCHAR sharedPaddingBuffer;

    struct
    {
        WDFSPINLOCK WdfRecvSpinLock;
        struct _WMBCLASS_NETADAPTER_CONTEXT* NetAdapterContext; // the context of the netAdapter instance if this session is in use
    } Sessions[MBB_MAX_NUMBER_OF_SESSIONS];

} WMBCLASS_DEVICE_CONTEXT, *PWMBCLASS_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WMBCLASS_DEVICE_CONTEXT, WmbClassGetDeviceContext);

EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_SURPRISE_REMOVAL EvtDeviceSurpriseRemoval;

EVT_MBB_DEVICE_SEND_MBIM_FRAGMENT EvtMbbDeviceSendMbimFragment;
EVT_MBB_DEVICE_RECEIVE_MBIM_FRAGMENT EvtMbbDeviceReceiveMbimFragment;
EVT_MBB_DEVICE_CREATE_ADAPTER EvtMbbDeviceCreateAdapter;
//EVT_NET_DEVICE_RESET EvtMbbDeviceReset;

EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtAdapterCleanup;
