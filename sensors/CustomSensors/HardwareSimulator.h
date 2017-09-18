//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//
//    This module contains the type definitions for the custom sensor sample
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

#define SIMULATOR_HARDWARE_INTERVAL_MS  (100) // 100ms

typedef enum _SIMULATOR_STATE
{
    SimulatorState_NotInitialized = 0,
    SimulatorState_Initialized,
    SimulatorState_Started
} SIMULATOR_STATE;

typedef class _HardwareSimulator
{
private:
    WDFTIMER            m_Timer;
    ULONG               m_Index;
    WDFWAITLOCK         m_Lock;
    SIMULATOR_STATE     m_State;
    WDFOBJECT           m_SimulatorInstance;

public:
    _HardwareSimulator();
    ~_HardwareSimulator();

    // WDF callbacks
    static EVT_WDF_TIMER        OnTimerExpire;

    static NTSTATUS Initialize(_In_ WDFDEVICE Device, _Out_ WDFOBJECT *SimulatorInstance);
    NTSTATUS Cleanup();
    NTSTATUS Start();
    NTSTATUS Stop();
    FLOAT GetSample();

private:
    NTSTATUS InitializeInternal(_In_ WDFOBJECT SimulatorInstance);

} HardwareSimulator, *PHardwareSimulator;

// Set up accessor function to retrieve device context
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HardwareSimulator, GetHardwareSimulatorContextFromInstance);
