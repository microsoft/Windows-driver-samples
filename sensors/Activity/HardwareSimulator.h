//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//    This module contains the type definitions for the activity sample driver hardware simulator.
//
//Environment:
//    Windows User-Mode Driver Framework (UMDF)

#pragma once

#include "Device.h"

typedef enum SIMULATOR_STATE
{
    SimulatorState_NotInitialized = 0,
    SimulatorState_Initialized,
    SimulatorState_Started
} SIMULATOR_STATE;

typedef class HardwareSimulator
{
private:
    WDFTIMER            m_Timer;
    ULONG               m_Index;
    WDFWAITLOCK         m_Lock;
    SIMULATOR_STATE     m_State;

public:
    HardwareSimulator();
    ~HardwareSimulator();

    // WDF callbacks
    static EVT_WDF_TIMER        OnTimerExpire;

    static NTSTATUS Initialize(_In_ WDFDEVICE device, _Out_ WDFOBJECT *pSimulatorInstance);
    VOID Deinitialize();
    VOID Start();
    VOID Stop();
    NTSTATUS GetSample(_Inout_ PSENSOR_COLLECTION_LIST pSample);
    NTSTATUS GetSample(_Out_ PActivitySample pSample);

private:
    NTSTATUS InitializeInternal(_In_ WDFOBJECT simulatorInstance);

} HardwareSimulator, *PHardwareSimulator;


// Set up accessor function to retrieve device context
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HardwareSimulator, GetHardwareSimulatorContextFromInstance);
