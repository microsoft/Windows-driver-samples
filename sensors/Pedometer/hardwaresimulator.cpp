//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of the pedometer sample driver
//    hardware simulator.
//
//Environment:
//
//    Windows User-Mode Driver Framework (UMDF)

#include "HardwareSimulator.h"

#include "HardwareSimulator.tmh"

#include "Device.h"

// Simulated pedometer data
// The simulation data represent the pedometer data for a user walking and running at different paces
// Since the simulator is designed to report a sample every second,
// each line in the table represents 1 second of data
const PedometerSample SimulatorData[] = {
//      1: Timestamp
//      2: IsFirstSample
//      3: Unknown step count
//      4: Unknown step duration in milliseconds
//      5: Walking step count
//      6: Walking step duration in milliseconds
//      7: Running step count
//      8: Running step duration in milliseconds
//
//      | 1 |  2  | 3 | 4 |  5 |   6 |  7 |     8 |
        { {}, TRUE,  0, 0,   0,     0,   0,     0 }, // the pedometer is reset
        { {}, FALSE, 0, 0,   0,     0,   0,     0 },
        { {}, FALSE, 0, 0,   1,  1000,   0,     0 }, // 3 seconds, The user starts walking
        { {}, FALSE, 0, 0,   3,  2000,   0,     0 },
        { {}, FALSE, 0, 0,   5,  3000,   0,     0 },
        { {}, FALSE, 0, 0,   7,  4000,   0,     0 },
        { {}, FALSE, 0, 0,   9,  5000,   0,     0 },
        { {}, FALSE, 0, 0,  12,  6000,   0,     0 }, // 8 seconds, the user starts accelerating the foot pace, the sensor hasn't detected a running pace yet 
        { {}, FALSE, 0, 0,  12,  6000,   3,  1000 }, // 9 seconds, the sensors detects the user is running
        { {}, FALSE, 0, 0,  12,  6000,   6,  2000 },
        { {}, FALSE, 0, 0,  12,  6000,  10,  3000 },
        { {}, FALSE, 0, 0,  12,  6000,  14,  4000 },
        { {}, FALSE, 0, 0,  12,  6000,  18,  5000 },
        { {}, FALSE, 0, 0,  12,  6000,  22,  6000 },
        { {}, FALSE, 0, 0,  12,  6000,  25,  7000 }, // 15 seconds, the user starts decelerating
        { {}, FALSE, 0, 0,  12,  6000,  27,  8000 }, 
        { {}, FALSE, 0, 0,  12,  6000,  28,  9000 },
        { {}, FALSE, 0, 0,  13,  7000,  28,  9000 }, // 18 seconds, the user walks again
        { {}, FALSE, 0, 0,  14,  8000,  28,  9000 },
        { {}, FALSE, 0, 0,  15,  9000,  28,  9000 },
        { {}, FALSE, 0, 0,  16, 10000,  28,  9000 },
        { {}, FALSE, 0, 0,  16, 10000,  28,  9000 }, // 22 seconds, the user stops walking
        { {}, FALSE, 0, 0,  16, 10000,  28,  9000 },
        { {}, FALSE, 0, 0,  16, 10000,  28,  9000 },
};

HardwareSimulator::HardwareSimulator() :
    m_HasReset(TRUE),
    m_Index(0),
    m_Lock(NULL),
    m_State(SimulatorState_NotInitialized),
    m_SimulatorInstance(NULL),
    m_Timer(NULL),
    m_HistoryIntervalInMs(0),
    m_History({})
{
}

HardwareSimulator::~HardwareSimulator()
{
}

// This static routine performs simulator initialization. The routine creates a 
// timer object that periodically updates the m_Index location 
NTSTATUS
HardwareSimulator::Initialize(
    _In_ WDFDEVICE Device,               // WDF device representing the sensor
    _Out_ WDFOBJECT *SimulatorInstance)   // Instance of the WDF object for the simulator
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
        TraceError("PED %!FUNC! WdfObjectCreate failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(*SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator->InitializeInternal(*SimulatorInstance);

Exit:

    SENSOR_FunctionExit(Status);

    return Status;
}


// Internal routine to perform simulator initialization
NTSTATUS
HardwareSimulator::InitializeInternal(
    _In_ WDFOBJECT SimulatorInstance)    // Instance of the WDF object for the simulator
{
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES TimerAttributes = {};
    WDF_TIMER_CONFIG TimerConfig = {};

    SENSOR_FunctionEnter();

    // Only initialize the simulator if it is in the "not initialized" state
    if (SimulatorState_NotInitialized == m_State)
    {
        // Create sample Lock
        Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
        if (!NT_SUCCESS(Status))
        {
            m_Lock = NULL;

            TraceError("PED %!FUNC! WdfWaitLockCreate for m_Lock failed %!STATUS!", Status);
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

            TraceError("PED %!FUNC! WdfTimerCreate failed %!STATUS!", Status);
            goto Exit;
        }

        // Create history lock
        Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_HistoryLock);
        if (!NT_SUCCESS(Status))
        {
            TraceError("PED %!FUNC! WdfWaitLockCreate failed %!STATUS!", Status);
            goto Exit;
        }

        m_HistoryIntervalInMs = Pedometer_Default_HistoryInterval_Ms;

        // Initialize history buffer
        m_History.pData = reinterpret_cast<PPedometerSample>(malloc(sizeof(PedometerSample) * Pedometer_Default_MaxHistoryEntries));
        if (nullptr == m_History.pData)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("PED %!FUNC! Allocating circular buffer for history failed %!STATUS!", Status);
            goto Exit;
        }
        m_History.FirstElemIndex = 0;
        m_History.LastElemIndex = 0;
        m_History.NumOfElems = 0;
        m_History.BufferLength = Pedometer_Default_MaxHistoryEntries;


        // Create an auto-reset event for signaling the busy read to stop
        m_HistoryCancelReadEvt = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL == m_HistoryCancelReadEvt)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("PED %!FUNC! Failed to create an event %!STATUS!", Status);
            goto Exit;
        }

        // Create timer object for keeping history
        WDF_TIMER_CONFIG_INIT(&TimerConfig, HardwareSimulator::OnHistoryTimerExpire);
        WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
        TimerAttributes.ParentObject = SimulatorInstance;
        TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

        Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_HistoryTimer);
        if (!NT_SUCCESS(Status))
        {
            TraceError("PED %!FUNC! WdfTimerCreate for history failed %!STATUS!", Status);
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
NTSTATUS
HardwareSimulator::Cleanup()
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

    // Close handle to Read cancellation event
    if (NULL != m_HistoryCancelReadEvt)
    {
        CloseHandle(m_HistoryCancelReadEvt);
        m_HistoryCancelReadEvt = NULL;
    }


    m_History.FirstElemIndex = 0;
    m_History.LastElemIndex = 0;
    m_History.NumOfElems = 0;
    m_History.BufferLength = 0;

    // Delete history buffer
    if (nullptr != m_History.pData)
    {
        free(m_History.pData);
        m_History.pData = nullptr;
    }

    // Delete history lock
    if (NULL != m_HistoryLock)
    {
        WdfObjectDelete(m_HistoryLock);
        m_HistoryLock = NULL;
    }

    // Set the simulator state to "not initialized"
    m_State = SimulatorState_NotInitialized;

    return status;
}


// This routine starts the simulator
NTSTATUS
HardwareSimulator::Start()
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (SimulatorState_Initialized == m_State)
    {
        WdfTimerStart(m_Timer, WDF_REL_TIMEOUT_IN_MS(SIMULATOR_HARDWARE_INTERVAL_MS));
        m_State = SimulatorState_Started;
    }

    SENSOR_FunctionExit(status);

    return status;
}


// This routine stops the simulator
NTSTATUS
HardwareSimulator::Stop()
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
VOID
HardwareSimulator::OnTimerExpire(
    _In_ WDFTIMER Timer) // WDF timer object
{
    HardwareSimulator *pSimulator = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    pSimulator = GetHardwareSimulatorContextFromInstance(WdfTimerGetParentObject(Timer));
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
    }

    if (NT_SUCCESS(Status))
    {
        // Increment the sample index, roll over if the index reach the end of the array
        WdfWaitLockAcquire(pSimulator->m_Lock, NULL);
        pSimulator->m_Index++;
        pSimulator->m_Index = pSimulator->m_Index % ARRAYSIZE(SimulatorData);

        if (FALSE != SimulatorData[pSimulator->m_Index].IsFirstAfterReset)
        {
            pSimulator->m_HasReset = TRUE;
        }

        WdfWaitLockRelease(pSimulator->m_Lock);

        WdfTimerStart(pSimulator->m_Timer, WDF_REL_TIMEOUT_IN_MS(SIMULATOR_HARDWARE_INTERVAL_MS));
    }

    SENSOR_FunctionExit(Status);
}


// This routine returns the current sample from the driver at the current m_Index
// location
NTSTATUS
HardwareSimulator::GetSample(
    _Out_ PedometerSample *Sample) // Pedometer sample
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == Sample)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! Sample parameter is null");
    }

    if (NT_SUCCESS(Status))
    {
        WdfWaitLockAcquire(m_Lock, NULL);
        *Sample = SimulatorData[m_Index];

        // The IsFirstAfterReset value should only be true for the first sample after a pedometer reset.
        // (Simulator specific) The below makes sure this requirement is respected when multiple calls to GetSample() 
        // happen in a shorter time than is required by the simulator to switch to the next sample (i.e. if multiple calls
        // to GetSample() happen within the same second).
        if (FALSE != m_HasReset)
        {
            Sample->IsFirstAfterReset = TRUE;
            m_HasReset = FALSE;
        }
        else
        {
            Sample->IsFirstAfterReset = FALSE;
        }

        WdfWaitLockRelease(m_Lock);

        GetSystemTimePreciseAsFileTime(&Sample->Timestamp);
    }

    SENSOR_FunctionExit(Status);

    return Status;
}


// This routine resets the pedometer
NTSTATUS
HardwareSimulator::Reset()
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    WdfWaitLockAcquire(m_Lock, NULL);
    m_Index = 0;
    WdfWaitLockRelease(m_Lock);

    SENSOR_FunctionExit(Status);

    return Status;
}


// This routine is called by history retrieval thread to remove an entry from the history buffer.
// Note this function must be called under lock
_Requires_lock_held_(m_HistoryLock)
NTSTATUS
HardwareSimulator::RemoveDataElemFromHistoryBuffer(
    _Out_ PPedometerSample pData // Pedometer data removed from the buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (0 == m_History.NumOfElems)
    {
        // buffer empty
        Status = STATUS_NO_MORE_ENTRIES;
    }
    else
    {
        *pData = m_History.pData[m_History.FirstElemIndex];
        m_History.FirstElemIndex++;
        m_History.FirstElemIndex %= m_History.BufferLength;
        m_History.NumOfElems--;
        if (0 == m_History.NumOfElems)
        {
            // Buffer Empty. 'LastElemIndex' should be same as 'FirstElemIndex'
            m_History.LastElemIndex = m_History.FirstElemIndex;
        }
    }

    return Status;
}



// This routine is called by worker thread to add an entry to the history buffer.
// Note this function must be called under lock
_Requires_lock_held_(m_HistoryLock)
NTSTATUS
HardwareSimulator::AddDataElemToHistoryBuffer(
    _In_ PPedometerSample pData // Pedometer data to be added to the buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (0 == m_History.NumOfElems)
    {
        // buffer empty
        m_History.NumOfElems++;
    }
    else if (m_History.BufferLength > m_History.NumOfElems)
    {
        // buffer not full yet. Increment the index of the last element in the circular buffer
        m_History.LastElemIndex++;
        m_History.LastElemIndex %= m_History.BufferLength;
        // Increment the num of elements
        m_History.NumOfElems++;
    }
    else if (m_History.BufferLength == m_History.NumOfElems)
    {
        // buffer full. Over-write the oldest element in the circular buffer
        m_History.FirstElemIndex++;
        m_History.FirstElemIndex %= m_History.BufferLength;
        m_History.LastElemIndex++;
        m_History.LastElemIndex %= m_History.BufferLength;
    }

    m_History.pData[m_History.LastElemIndex] = *pData;

    return Status;
}



// This callback is called when interval wait time has expired and driver is ready
// to collect new sample. The callback stores pedometer data in history buffer,
// and schedules next wake up time.
VOID
HardwareSimulator::OnHistoryTimerExpire(
    _In_ WDFTIMER HistoryTimer // WDF timer object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;
    PedometerSample Sample = {};

    SENSOR_FunctionEnter();

    pSimulator = GetHardwareSimulatorContextFromInstance(WdfTimerGetParentObject(HistoryTimer));

    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Just use the current sample to store in the history buffer
    Status = pSimulator->GetSample(&Sample);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! GetSample failed %!STATUS!", Status);
        goto Exit;
    }

    WdfWaitLockAcquire(pSimulator->m_HistoryLock, NULL);

    // Add data to the buffer
    Status = pSimulator->AddDataElemToHistoryBuffer(&Sample);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! AddDataElemToHistoryBuffer Failed %!STATUS!", Status);
    }
    WdfWaitLockRelease(pSimulator->m_HistoryLock);

    // Schedule next wake up time
    if (FALSE != pSimulator->m_HistoryStarted)
    {
        WdfTimerStart(pSimulator->m_HistoryTimer, WDF_REL_TIMEOUT_IN_MS(pSimulator->m_HistoryIntervalInMs));
    }

Exit:

    SENSOR_FunctionExit(Status);
}

// This routine starts a timer that periodically records the samples to History
NTSTATUS
HardwareSimulator::StartHistory()
{
    NTSTATUS Status = STATUS_SUCCESS;

    WdfWaitLockAcquire(m_HistoryLock, NULL);

    if (FALSE != m_HistoryStarted)
    {
        Status = STATUS_DEVICE_BUSY;
        TraceError("PED %!FUNC! History Collection is already started %!STATUS!", Status);
        goto Exit;
    }

    // Start keeping history
    m_HistoryStarted = TRUE;

    // Start timer
    WdfTimerStart(m_HistoryTimer, WDF_REL_TIMEOUT_IN_MS(m_HistoryIntervalInMs));

Exit:

    WdfWaitLockRelease(m_HistoryLock);
    return Status;
}

// This routine stops the timer that is responsible for history collection.
NTSTATUS
HardwareSimulator::StopHistory()
{
    WdfWaitLockAcquire(m_HistoryLock, NULL);

    // Stop collecting history
    m_HistoryStarted = FALSE;

    // Stop timer
    WdfTimerStop(m_HistoryTimer, TRUE);

    WdfWaitLockRelease(m_HistoryLock);

    return STATUS_SUCCESS;
}

// This routine clears the history collected thus far
NTSTATUS 
HardwareSimulator::ClearHistory()
{
    NTSTATUS Status = STATUS_SUCCESS;

    WdfWaitLockAcquire(m_HistoryLock, NULL);

    m_History.FirstElemIndex = 0;
    m_History.LastElemIndex = 0;
    m_History.NumOfElems = 0;
    RtlZeroMemory(m_History.pData, (m_History.BufferLength*sizeof(PedometerSample)));

    // Clearing History should reset the Pedometer
    Status = Reset();

    WdfWaitLockRelease(m_HistoryLock);

    return Status;

}

// This routine reads the history collected so far up to a maximum of 'BufferSize' records.
// Once read, those samples will be removed from the History.
// All read samples will be removed from the History buffer
// Any unread samples will continue to persist int the History buffer
NTSTATUS 
HardwareSimulator::ReadHistory(
    _Inout_ PULONG SamplesCount,
    _Out_writes_to_(*SamplesCount, *SamplesCount) PPedometerSample HistorySamplesBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SamplesCopied = 0;

    // Make sure we have sufficient memory to fill in the samples
    if (0 == *SamplesCount)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    while (*SamplesCount > SamplesCopied)
    {
        PedometerSample Data = {};

        // Check whether the Cancel event is signaled before looping through each time to retrieve an entry from the history buffer.
        if (WAIT_OBJECT_0 == WaitForSingleObjectEx(m_HistoryCancelReadEvt, 0, FALSE))
        {
            TraceError("PED %!FUNC! Read canceled");
            Status = STATUS_CANCELLED;
            break;
        }

        WdfWaitLockAcquire(m_HistoryLock, NULL);
        Status = RemoveDataElemFromHistoryBuffer(&Data);
        WdfWaitLockRelease(m_HistoryLock);

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        HistorySamplesBuffer[SamplesCopied++] = Data;
    }

    if (STATUS_NO_MORE_ENTRIES == Status)
    {
        // Ignore STATUS_NO_MORE_ENTRIES if there were any entries that were copied
        if (0 < SamplesCopied)
        {
            Status = STATUS_SUCCESS;
        }
    } 
    else if (*SamplesCount < SamplesCopied)
    {
        Status = STATUS_BUFFER_OVERFLOW;
    }

Exit:
    *SamplesCount = SamplesCopied;
    return Status;
}

// This routine sets an event for the ReadHistory function to exit
// This is for illustration purpose only. For real HW, use an appropriate method to cancel the pending reads.
NTSTATUS
HardwareSimulator::SignalReadCancellation()
{
    SetEvent(m_HistoryCancelReadEvt);
    return STATUS_SUCCESS;
}

