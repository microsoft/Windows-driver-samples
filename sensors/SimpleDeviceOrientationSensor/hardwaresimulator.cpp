//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of the simple device orientation sensor sample
//    hardware simulator.
//
//Environment:
//
//    Windows User-Mode Driver Framework (UMDF)

#include "HardwareSimulator.h"

#include "HardwareSimulator.tmh"

// Simulated device orientations
const ABI::Windows::Devices::Sensors::SimpleOrientation OrientationData[] = {
    ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Facedown,
    ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_NotRotated,
    ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Faceup,
    ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Rotated90DegreesCounterclockwise,
    ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Facedown,
    ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Rotated180DegreesCounterclockwise,
    ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Faceup,
    ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Rotated270DegreesCounterclockwise
};

HardwareSimulator::_HardwareSimulator() :
    m_Index(0),
    m_Lock(NULL),
    m_State(SimulatorState_NotInitialized),
    m_SimulatorInstance(NULL),
    m_Timer(NULL)
{
}

HardwareSimulator::~_HardwareSimulator()
{
}

// This static routine performs simulator initialization. The routine creates a 
// timer object that periodically updates the m_Index location 
// Returns an NTSTATUS code
NTSTATUS HardwareSimulator::Initialize(
    _In_ WDFDEVICE Device,                  // WDF device representing the sensor
    _Out_ WDFOBJECT *SimulatorInstance)     // Instance of the WDF object for the simulator
{
    PHardwareSimulator pSimulator = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES HardwareSimulatorAttributes = {};

    SENSOR_FunctionEnter();

    // Create WDFOBJECT for the hardware simulator
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&HardwareSimulatorAttributes, HardwareSimulator);
    HardwareSimulatorAttributes.ParentObject = Device;

    Status = WdfObjectCreate(&HardwareSimulatorAttributes, SimulatorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("SDOS %!FUNC! WdfObjectCreate failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(*SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("SDOS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator->InitializeInternal(*SimulatorInstance);

Exit:

    SENSOR_FunctionExit(Status);

    return Status;
}

// Internal routine to perform simulator initialization
// Returns an NTSTATUS code
NTSTATUS HardwareSimulator::InitializeInternal(
    _In_ WDFOBJECT SimulatorInstance)    // Instance of the WDF object for the simulator
{
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES TimerAttributes = {};
    WDF_TIMER_CONFIG TimerConfig = {};

    SENSOR_FunctionEnter();

    // Only initialize the simulator if it is in the "not initialized" state
    if (SimulatorState_NotInitialized == m_State)
    {
        // Create Lock
        Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
        if (!NT_SUCCESS(Status))
        {
            m_Lock = NULL;

            TraceError("SDOS %!FUNC! WdfWaitLockCreate failed %!STATUS!", Status);
            goto Exit;
        }

        // Create a timer object for simulation updates
        WDF_TIMER_CONFIG_INIT(&TimerConfig, HardwareSimulator::OnTimerExpire);
        WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
        TimerAttributes.ParentObject = SimulatorInstance;
        TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

        Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_Timer);
        if (!NT_SUCCESS(Status))
        {
            m_Timer = NULL;

            TraceError("SDOS %!FUNC! WdfTimerCreate failed %!STATUS!", Status);
            goto Exit;
        }

        // Set the simulator state to "initialized"
        m_State = SimulatorState_Initialized;
        m_SimulatorInstance = SimulatorInstance;
    }

Exit:
    if (!NT_SUCCESS(Status) && NULL != m_Lock)
    {
        WdfObjectDelete(m_Lock);
        m_Lock = NULL;
    }

    SENSOR_FunctionExit(Status);

    return Status;
}

// This routine perform a simulator cleanup
// Returns an NTSTATUS code
NTSTATUS HardwareSimulator::Cleanup()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (SimulatorState_Started == m_State)
    {
        Stop();
    }

    // Delete lock
    if (NULL != m_Lock)
    {
        WdfObjectDelete(m_Lock);
        m_Lock = NULL;
    }

    // Set the simulator state to "not initialized"
    m_State = SimulatorState_NotInitialized;

    return status;
}

// This routine starts the simulator
// Returns an NTSTATUS code
NTSTATUS HardwareSimulator::Start()
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (SimulatorState_Initialized == m_State)
    {
        WdfTimerStart(m_Timer, WDF_REL_TIMEOUT_IN_MS(HardwareSimulator_HardwareInterval));
        m_State = SimulatorState_Started;
    }

    SENSOR_FunctionExit(status);

    return status;
}

// This routine stops the simulator
// Returns an NTSTATUS code
NTSTATUS HardwareSimulator::Stop()
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (SimulatorState_Started == m_State)
    {
        WdfTimerStop(m_Timer, TRUE);
        m_State = SimulatorState_Initialized;
    }

    SENSOR_FunctionExit(status);

    return status;
}

// This callback is called when the simulator wait time has expired and the simulator
// is ready to switch to the next sample. The callback updates the sample index and 
// schedules the next wake up time.
VOID HardwareSimulator::OnTimerExpire(
    _In_ WDFTIMER Timer) // WDF timer object
{
    HardwareSimulator *pSimulator = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    pSimulator = GetHardwareSimulatorContextFromInstance(WdfTimerGetParentObject(Timer));
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("SDOS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
    }

    if (NT_SUCCESS(Status))
    {
        // Increment the sample index, roll over if the index reach the end of the array
        WdfWaitLockAcquire(pSimulator->m_Lock, NULL);
        pSimulator->m_Index++;
        pSimulator->m_Index %= ARRAYSIZE(OrientationData);
        WdfWaitLockRelease(pSimulator->m_Lock);

        WdfTimerStart(pSimulator->m_Timer, WDF_REL_TIMEOUT_IN_MS(HardwareSimulator_HardwareInterval));
    }

    SENSOR_FunctionExit(Status);
}

// This routine returns the current sample from the driver at the current m_Index location.
// Returns one of the ABI::Windows::Devices::Sensors::SimpleOrientation enum values.
ABI::Windows::Devices::Sensors::SimpleOrientation HardwareSimulator::GetOrientation()
{
    ABI::Windows::Devices::Sensors::SimpleOrientation Sample = ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Faceup;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    WdfWaitLockAcquire(m_Lock, NULL);
    Sample = OrientationData[m_Index];
    WdfWaitLockRelease(m_Lock);

    SENSOR_FunctionExit(Status);

    return Sample;
}