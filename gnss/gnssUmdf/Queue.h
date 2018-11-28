/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    queue.h

Abstract:

    This file contains the queue definitions for the Umdf Gnss Driver.

Environment:

    Windows User-Mode Driver Framework

--*/

#pragma once

class CFixHandler;

class CQueue
{
public:
    static NTSTATUS AddQueueToDevice(_In_ WDFDEVICE device, _Outptr_ CQueue** queue);

private:
    enum
    {
        FAKE_AGNSS_TIME_IDX = GNSS_AGNSS_TimeInjection,
        FAKE_AGNSS_POS_IDX = GNSS_AGNSS_PositionInjection,
        FAKE_AGNSS_BLOB_IDX = GNSS_AGNSS_BlobInjection,
        FAKE_AGNSS_MAX_IDX
    };

    typedef struct {
        DWORD OperationMode;
        BOOL SetLocationNiRequestAllowed;
        BOOL SetLocationServiceEnabled;
    } FakeGnssStateDump;

    WDFQUEUE _Queue;
    CRITICAL_SECTION _Lock;
    CFixHandler _FixHandler;
    ULONG _ForcedDriverVersion = GNSS_DRIVER_DDK_VERSION;
    FakeGnssStateDump _InternalState;
    BOOL _AgnssNeeded[FAKE_AGNSS_MAX_IDX];
    
    CQueue(WDFQUEUE Queue);
    ~CQueue();

    static EVT_WDF_OBJECT_CONTEXT_CLEANUP OnCleanup;
    static EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL OnIoDeviceControl;

    NTSTATUS Initialize();
    void OnIoDeviceControl(_In_ WDFREQUEST Request,
                           _In_ ULONG IoControlCode,
                           _In_ size_t InputBufferLength,
                           _In_ size_t OutputBufferLength);
    NTSTATUS GetDeviceCapability(_In_ WDFREQUEST Request);
    NTSTATUS GetChipsetInfo(_In_ WDFREQUEST Request);
    NTSTATUS HandleDriverCommand(_In_ WDFREQUEST Request);
    NTSTATUS ClearAgnssData(_In_ PGNSS_DRIVERCOMMAND_PARAM CommandParam);
    NTSTATUS ResetEngine(_In_ PGNSS_DRIVERCOMMAND_PARAM CommandParam);
    NTSTATUS SetLocationServiceStatus(_In_ PGNSS_DRIVERCOMMAND_PARAM CommandParam);
    NTSTATUS ForceOperationMode(_In_ PGNSS_DRIVERCOMMAND_PARAM CommandParam);
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CQueue, GetQueueObject);
