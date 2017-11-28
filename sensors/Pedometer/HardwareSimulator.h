//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//
//    This module contains the type definitions for the pedometer sample driver
//    hardware simulator.
//
//Environment:
//
//    Windows User-Mode Driver Framework (UMDF)

#pragma once

#include <windows.h>
#include <wdf.h>

#include "SensorsTrace.h"
#include <SensorsCx.h>

#include "Device.h"

#define SIMULATOR_HARDWARE_INTERVAL_MS    (1000)  // 1 second interval in milliseconds

typedef enum SIMULATOR_STATE
{
    SimulatorState_NotInitialized = 0,
    SimulatorState_Initialized,
    SimulatorState_Started
} SIMULATOR_STATE;

typedef class HistoryIterator
{
public:
    HistoryIterator();
    ~HistoryIterator();

    NTSTATUS Next(_Out_ PedometerSample *Sample);

}HistoryIterator, *PHistoryIterator;

typedef class HardwareSimulator
{
private:
    WDFTIMER                m_Timer;
    ULONG                   m_Index;
    WDFWAITLOCK             m_Lock;
    SIMULATOR_STATE         m_State;
    WDFOBJECT               m_SimulatorInstance;
    BOOLEAN                 m_HasReset;

    // History operation
    WDFWAITLOCK             m_HistoryLock;
    WDFTIMER                m_HistoryTimer;
    HistoryCircBuffer       m_History;
    ULONG                   m_HistoryIntervalInMs;
    BOOLEAN                 m_HistoryStarted;
    HANDLE                  m_HistoryCancelReadEvt;

public:
    HardwareSimulator();
    ~HardwareSimulator();

    // WDF callbacks
    static EVT_WDF_TIMER        OnTimerExpire;

    static NTSTATUS Initialize(_In_ WDFDEVICE Device, _Out_ WDFOBJECT *SimulatorInstance);
    NTSTATUS Cleanup();
    NTSTATUS Start();
    NTSTATUS Stop();
    NTSTATUS GetSample(_Out_ PedometerSample *Sample);
    NTSTATUS Reset();
    NTSTATUS ClearHistory();
    NTSTATUS StartHistory();
    NTSTATUS StopHistory();
    NTSTATUS ReadHistory(_Inout_ PULONG SamplesCount, _Out_writes_to_(*SamplesCount, *SamplesCount) PPedometerSample HistorySamplesBuffer);
    NTSTATUS SignalReadCancellation();

    static EVT_WDF_TIMER OnHistoryTimerExpire;

    ULONG GetHistorySizeInRecords()
    {
        return m_History.BufferLength;
    }

    ULONG GetHistoryIntervalInMs()
    {
        return m_HistoryIntervalInMs;
    }

private:
    NTSTATUS InitializeInternal(_In_ WDFOBJECT SimulatorInstance);
    NTSTATUS AddHistoryEntry(_In_ PedometerSample& Sample);
    _Requires_lock_held_(m_HistoryLock)
    NTSTATUS AddDataElemToHistoryBuffer(_In_ PPedometerSample pData);
    _Requires_lock_held_(m_HistoryLock)
    NTSTATUS RemoveDataElemFromHistoryBuffer(_Out_ PPedometerSample pData);

} HardwareSimulator, *PHardwareSimulator;


// Set up accessor function to retrieve device context
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HardwareSimulator, GetHardwareSimulatorContextFromInstance);
