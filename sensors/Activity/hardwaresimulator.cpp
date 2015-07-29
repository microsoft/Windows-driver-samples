//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//    This module contains the implementation of the activity sample driver hardware simulator.
//
//Environment:
//    Windows User-Mode Driver Framework (UMDF)

#include "HardwareSimulator.h"
#include "HardwareSimulator.tmh"

static const ULONG SIMULATOR_HARDWARE_INTERVAL_SEC = 10;
static const ULONG MAX_ACTIVITY_STATE_PER_SAMPLE = 4;

// Simulated activity data
const ActivitySample c_SimulatorData[][MAX_ACTIVITY_STATE_PER_SAMPLE] = {
    // This is a 2D array. Each row is one activity sample. One activity sample consist one or more
    // state and confidence pair.
    { { {}, ActivityState_InVehicle, 40 }, { {}, ActivityState_Biking,    30 }, { {}, ActivityState_Walking,   20 }, {                               } }, // InVehicle 40%, Biking 30%, Walking 20%
    { { {}, ActivityState_Biking,    45 }, { {}, ActivityState_InVehicle, 40 }, { {}, ActivityState_Fidgeting, 30 }, { {}, ActivityState_Walking, 20 } }, // Biking 45%, InVehicle 40% Fidgeting 30%, Walking 20%
    { { {}, ActivityState_Idle,      80 }, { {}, ActivityState_InVehicle, 60 }, { {}, ActivityState_Biking,    35 }, { {}, ActivityState_Walking, 20 } }, // Idle 80%, InVehicle 60%, Biking 35%, Walking 20%
    { { {}, ActivityState_InVehicle, 75 }, { {}, ActivityState_Idle,      55 }, { {}, ActivityState_Walking,   18 }, {                               } }, // InVehicle 75%, Idle 55%, Walking 18%
    { { {}, ActivityState_Biking,    90 }, {                                 }, {                                 }, {                               } }, // Biking 90%
};

HardwareSimulator::HardwareSimulator() :
    m_Index(0),
    m_Lock(NULL),
    m_State(SimulatorState_NotInitialized),
    m_Timer(NULL)
{
}

HardwareSimulator::~HardwareSimulator()
{
}

// This static routine performs simulator initialization. The routine creates a 
// timer object that periodically updates the m_Index location 
NTSTATUS HardwareSimulator::Initialize(
    _In_ WDFDEVICE device,                 // WDF device representing the sensor
    _Out_ WDFOBJECT *pSimulatorInstance)   // Instance of the WDF object for the simulator
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    *pSimulatorInstance = NULL;

    // Create WDFOBJECT for the hardware simulator
    WDF_OBJECT_ATTRIBUTES hardwareSimulatorAttributes = {};
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&hardwareSimulatorAttributes, HardwareSimulator);
    hardwareSimulatorAttributes.ParentObject = device;

    status = WdfObjectCreate(&hardwareSimulatorAttributes, pSimulatorInstance);
    if (!NT_SUCCESS(status))
    {
        TraceError("ACT %!FUNC! WdfObjectCreate failed %!STATUS!", status);
    }
    else
    {
        PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(*pSimulatorInstance);
        if (nullptr == pSimulator)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("ACT %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", status);
        }
        else
        {
            status = pSimulator->InitializeInternal(*pSimulatorInstance);
        }
    }

    SENSOR_FunctionExit(status);

    return status;
}


// Internal routine to perform simulator initialization
NTSTATUS HardwareSimulator::InitializeInternal(_In_ WDFOBJECT SimulatorInstance)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    SENSOR_FunctionEnter();

    // Only initialize the simulator if it is in the "not initialized" state
    if (SimulatorState_NotInitialized == m_State)
    {
        // Create sample Lock
        status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
        if (!NT_SUCCESS(status))
        {
            m_Lock = NULL;
            TraceError("ACT %!FUNC! WdfWaitLockCreate for m_Lock failed %!STATUS!", status);
        }
        else
        {
            WDF_OBJECT_ATTRIBUTES timerAttributes = {};
            WDF_TIMER_CONFIG timerConfig = {};

            // Create a timer object for simulation updates
            WDF_TIMER_CONFIG_INIT(&timerConfig, HardwareSimulator::OnTimerExpire);
            WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
            timerAttributes.ParentObject = SimulatorInstance;
            timerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

            status = WdfTimerCreate(&timerConfig, &timerAttributes, &m_Timer);
            if (!NT_SUCCESS(status))
            {
                m_Timer = NULL;
                TraceError("ACT %!FUNC! WdfTimerCreate failed %!STATUS!", status);
            }
            else
            {
                // Set the simulator state to "initialized"
                m_State = SimulatorState_Initialized;
            }
        }
    }

    if (!NT_SUCCESS(status) && NULL != m_Lock)
    {
        WdfObjectDelete(m_Lock);
        m_Lock = NULL;
    }

    SENSOR_FunctionExit(status);

    return status;
}


// This routine perform a simulator cleanup
VOID HardwareSimulator::Deinitialize()
{
    if (SimulatorState_Started == m_State)
    {
        Stop();
    }

    // Delete lock. m_Timer will be deleted when the wdfobject is deleted
    if (NULL != m_Lock)
    {
        WdfObjectDelete(m_Lock);
        m_Lock = NULL;
    }

    // Set the simulator state to "not initialized"
    m_State = SimulatorState_NotInitialized;
}


// This routine starts the simulator
VOID HardwareSimulator::Start()
{
    SENSOR_FunctionEnter();

    if (SimulatorState_Initialized == m_State)
    {
        WdfTimerStart(m_Timer, WDF_REL_TIMEOUT_IN_SEC(SIMULATOR_HARDWARE_INTERVAL_SEC));
        m_State = SimulatorState_Started;
    }

    SENSOR_FunctionExit(STATUS_SUCCESS);
}


// This routine stops the simulator
VOID HardwareSimulator::Stop()
{
    SENSOR_FunctionEnter();

    if (SimulatorState_Started == m_State)
    {
        WdfTimerStop(m_Timer, TRUE);
        m_State = SimulatorState_Initialized;
    }

    SENSOR_FunctionExit(STATUS_SUCCESS);
}


// This callback is called when the simulator wait time has expired and the simulator
// is ready to switch to the next sample. The callback updates the sample index and 
// schedules the next wake up time.
VOID HardwareSimulator::OnTimerExpire(_In_ WDFTIMER timer)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(WdfTimerGetParentObject(timer));
    if (nullptr == pSimulator)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("ACT %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", status);
    }
    else
    {
        // Increment the sample index, roll over if the index reach the end of the array
        WdfWaitLockAcquire(pSimulator->m_Lock, NULL);
        pSimulator->m_Index++;
        pSimulator->m_Index = pSimulator->m_Index % ARRAYSIZE(c_SimulatorData);
        WdfWaitLockRelease(pSimulator->m_Lock);
        WdfTimerStart(pSimulator->m_Timer, WDF_REL_TIMEOUT_IN_SEC(SIMULATOR_HARDWARE_INTERVAL_SEC));
    }

    SENSOR_FunctionExit(status);
}


// This routine returns the current sample collection from the driver at the current m_Index location
NTSTATUS HardwareSimulator::GetSample(_Inout_ PSENSOR_COLLECTION_LIST pSample)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSample || 
        SENSOR_COLLECTION_LIST_SIZE(MAX_ACTIVITY_STATE_PER_SAMPLE * 2 + 1) > pSample->AllocatedSizeInBytes)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sample parameter is null");
    }
    else
    {
        pSample->Count = 1;

        WdfWaitLockAcquire(m_Lock, NULL);
        if (ARRAYSIZE(c_SimulatorData) > m_Index)
        {
            for (ULONG count = 0, index = 1;
                count < MAX_ACTIVITY_STATE_PER_SAMPLE && NULL != c_SimulatorData[m_Index][count].Activity;
                count++, index += 2)
            {
                pSample->List[index].Key = PKEY_SensorData_CurrentActivityState;
                InitPropVariantFromUInt32(c_SimulatorData[m_Index][count].Activity, &(pSample->List[index].Value));

                pSample->List[index + 1].Key = PKEY_SensorData_CurrentActivityStateConfidence_Percentage;
                InitPropVariantFromUInt16(c_SimulatorData[m_Index][count].Confidence, &(pSample->List[index + 1].Value));

                pSample->Count += 2;
            }
        }
        WdfWaitLockRelease(m_Lock);
    }

    SENSOR_FunctionExit(status);

    return status;
}

// This routine returns the current most probable sample from the driver at the current m_Index location
NTSTATUS HardwareSimulator::GetSample(_Out_ PActivitySample pSample)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSample)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sample parameter is null");
    }
    else
    {
        WdfWaitLockAcquire(m_Lock, NULL);
        *pSample = c_SimulatorData[m_Index][0];
        WdfWaitLockRelease(m_Lock);
    }

    SENSOR_FunctionExit(status);

    return status;
}