//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of driver callback function
//    from clx to pedometer.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "HardwareSimulator.h"

#include <timeapi.h>
#include <Intsafe.h>

#include "Client.tmh"

// This routine is called by worker thread to read a single sample, compare threshold
// and push it back to CLX. It simulates hardware thresholding by only generating data
// when the change of data is greater than threshold.
NTSTATUS
PedometerDevice::GetData(
)
{
    PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(m_SimulatorInstance);
    BOOLEAN DataReady = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CachedStepCountLimit = 0;
    ULONG LastStepCountLimit = 0;
    PedometerSample Sample = {};

    SENSOR_FunctionEnter();

    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    Status = pSimulator->GetSample(&Sample);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! GetSample failed %!STATUS!", Status);
        goto Exit;
    }

    if (FALSE != m_FirstSample)
    {
        Status = GetPerformanceTime(&m_StartTime);
        if (!NT_SUCCESS(Status))
        {
            m_StartTime = 0;
            TraceError("PED %!FUNC! GetPerformanceTime failed %!STATUS!", Status);
        }

        m_SampleCount = 0;

        DataReady = TRUE;
    }
    else
    {
        if (0 == m_CachedThreshold || FALSE != Sample.IsFirstAfterReset)
        {
            // Streaming mode
            DataReady = TRUE;
        }
        else
        {
            if (FAILED(ULongAdd(Sample.UnknownStepCount, Sample.WalkingStepCount, &CachedStepCountLimit)) ||
                FAILED(ULongAdd(Sample.RunningStepCount, CachedStepCountLimit, &CachedStepCountLimit)))
            {
                // If an overflow happened, we assume we reached the threshold
                // in other words, there is no threshold value that can be larger
                // than an overflowed value.
                DataReady = TRUE;
            }
            else if (FAILED(ULongAdd(m_LastSample.UnknownStepCount, m_LastSample.WalkingStepCount, &LastStepCountLimit)) ||
                     FAILED(ULongAdd(m_LastSample.RunningStepCount, LastStepCountLimit, &LastStepCountLimit)))
            {
                // If an overflow happened, we assume we reached the threshold
                // in other words, there is no threshold value that can be larger
                // than an overflowed value.
                DataReady = TRUE;
            }
            else if ((LastStepCountLimit < m_CachedThreshold && CachedStepCountLimit >= m_CachedThreshold) ||
                     (FALSE != Sample.IsFirstAfterReset))
            {
                // Compare the change of data to threshold, and only push the data back to 
                // clx if the change exceeds threshold or if this is the first sample after reset. This is usually done in HW.
                DataReady = TRUE;
            }
        }
    }

    if (FALSE != DataReady)
    {
        // update last sample
        m_LastSample = Sample;

        // push to clx
        InitPropVariantFromBoolean(m_LastSample.IsFirstAfterReset, &(m_pData->List[PEDOMETER_DATA_FIRST_AFTER_RESET].Value));
        InitPropVariantFromUInt32(PedometerStepType_Unknown, &(m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_TYPE].Value));
        InitPropVariantFromInt64(m_LastSample.UnknownStepDurationMs, &(m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_DURATION].Value));
        InitPropVariantFromUInt32(m_LastSample.UnknownStepCount, &(m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_COUNT].Value));
        InitPropVariantFromUInt32(PedometerStepType_Walking, &(m_pData->List[PEDOMETER_DATA_WALKING_STEP_TYPE].Value));
        InitPropVariantFromInt64(m_LastSample.WalkingStepDurationMs, &(m_pData->List[PEDOMETER_DATA_WALKING_STEP_DURATION].Value));
        InitPropVariantFromUInt32(m_LastSample.WalkingStepCount, &(m_pData->List[PEDOMETER_DATA_WALKING_STEP_COUNT].Value));
        InitPropVariantFromUInt32(PedometerStepType_Running, &(m_pData->List[PEDOMETER_DATA_RUNNING_STEP_TYPE].Value));
        InitPropVariantFromInt64(m_LastSample.RunningStepDurationMs, &(m_pData->List[PEDOMETER_DATA_RUNNING_STEP_DURATION].Value));
        InitPropVariantFromUInt32(m_LastSample.RunningStepCount, &(m_pData->List[PEDOMETER_DATA_RUNNING_STEP_COUNT].Value));

        // reset IsFirstAfterReset
        m_LastSample.IsFirstAfterReset = FALSE;

        InitPropVariantFromFileTime(&m_LastSample.Timestamp, &(m_pData->List[PEDOMETER_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("PED %!FUNC! Data did NOT meet the threshold");
    }

    SENSOR_FunctionExit(Status);

Exit:
    return Status;
}



// This callback is called when interval wait time has expired and driver is ready
// to collect new sample. The callback reads current value, compare value to threshold,
// pushes it up to CLX framework, and schedule next wake up time.
VOID
PedometerDevice::OnTimerExpire(
    _In_ WDFTIMER Timer // WDF timer object
    )
{
    PPedometerDevice pDevice = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    pDevice = GetPedometerContextFromSensorInstance(WdfTimerGetParentObject(Timer));
    if (nullptr == pDevice)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetPedometerContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Get data and push to clx
    WdfWaitLockAcquire(pDevice->m_Lock, NULL);
    Status = pDevice->GetData();
    if (!NT_SUCCESS(Status) && Status != STATUS_DATA_NOT_ACCEPTED)
    {
        TraceError("PED %!FUNC! GetData Failed %!STATUS!", Status);
    }
    WdfWaitLockRelease(pDevice->m_Lock);

    // Schedule next wake up time
    if (FALSE != pDevice->m_PoweredOn &&
        FALSE != pDevice->m_Started)
    {
        LONGLONG WaitTimeHundredNanoseconds = 0;  // in unit of 100ns

        if (0 == pDevice->m_StartTime)
        {
            // in case we fail to get sensor start time, use static wait time
            WaitTimeHundredNanoseconds = WDF_REL_TIMEOUT_IN_MS(pDevice->m_Interval);
        }
        else
        {
            ULONG CurrentTimeMs = 0;

            // dynamically calculate wait time to avoid jitter
            Status = GetPerformanceTime (&CurrentTimeMs);
            if (!NT_SUCCESS(Status))
            {
                TraceError("PED %!FUNC! GetPerformanceTime %!STATUS!", Status);
                WaitTimeHundredNanoseconds = WDF_REL_TIMEOUT_IN_MS(pDevice->m_Interval);
            }
            else
            {
                pDevice->m_SampleCount++;
                if (CurrentTimeMs > (pDevice->m_StartTime + (pDevice->m_Interval * (pDevice->m_SampleCount + 1))))
                {
                    // If we skipped two or more beats, reschedule the timer with a zero due time to catch up on missing samples
                    WaitTimeHundredNanoseconds = 0;
                }
                else
                {
                    WaitTimeHundredNanoseconds = (pDevice->m_StartTime +
                        (pDevice->m_Interval * (pDevice->m_SampleCount + 1))) - CurrentTimeMs;
                }
                WaitTimeHundredNanoseconds = WDF_REL_TIMEOUT_IN_MS(WaitTimeHundredNanoseconds);
            }
        }
        WdfTimerStart(pDevice->m_Timer, WaitTimeHundredNanoseconds);
    }

Exit:

    SENSOR_FunctionExit(Status);
}



// Called by Sensor CLX to begin continuously sampling the sensor.
NTSTATUS
PedometerDevice::OnStart(
    _In_ SENSOROBJECT SensorInstance // Sensor device object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    // Get the simulator context
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
    }

    if (NT_SUCCESS(Status))
    {
        // Start the simulator
        pSimulator->Start();

        pDevice->m_FirstSample = TRUE;

        // Start polling

        pDevice->m_Started = TRUE;

        InitPropVariantFromUInt32(SensorState_Active,
            &(pDevice->m_pProperties->List[SENSOR_PROPERTY_STATE].Value));

        // Start the sample polling timer.
        //
        // Note: The polling timer is configured to allow for the first sample to be reported immediately.
        // Some hardware may want to delay the first sample report a little to account for hardware start time.
        WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(Pedometer_Default_MinDataInterval_Ms));
    }
Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to stop continuously sampling the sensor.
NTSTATUS
PedometerDevice::OnStop(
    _In_ SENSOROBJECT SensorInstance // Sensor device object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();
    
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    // Stop polling
    pDevice->m_Started = FALSE;

    // Waiting for the callback to complete, then stopping the timer
    WdfTimerStop(pDevice->m_Timer, TRUE);

    InitPropVariantFromUInt32(SensorState_Idle, 
                              &(pDevice->m_pProperties->List[SENSOR_PROPERTY_STATE].Value));

    // Stop the simulator
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator->Stop();

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}


// Called by Sensor CLX to begin keeping history
NTSTATUS
PedometerDevice::OnStartHistory(
    _In_ SENSOROBJECT SensorInstance // Sensor device object
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;
    PHardwareSimulator pSimulator = nullptr;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    if (FALSE == pDevice->m_HistorySupported)
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("PED %!FUNC! History is not supported by the HW");
        goto Exit;
    }

    if (FALSE == pDevice->m_PoweredOn)
    {
        Status = STATUS_DEVICE_NOT_READY;
        TraceError("PED %!FUNC! Sensor is not powered on! %!STATUS!", Status);
        goto Exit;
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);

    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Start the pedometer history
    Status = pSimulator->StartHistory();
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! Start History failed %!STATUS!", Status);
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to stop keeping history.
NTSTATUS
PedometerDevice::OnStopHistory(
    _In_ SENSOROBJECT SensorInstance // Sensor device object
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;
    PHardwareSimulator pSimulator = nullptr;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    if (FALSE == pDevice->m_HistorySupported)
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("PED %!FUNC! History is not supported by the HW");
        goto Exit;
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);

    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Stop the pedometer history
    Status = pSimulator->StopHistory();
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! Stop History failed %!STATUS!", Status);
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}


// Resets the pedometer to its initial values.
VOID
PedometerDevice::ResetPedometer()
{
    NTSTATUS Status = STATUS_SUCCESS;
    PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(m_SimulatorInstance);

    SENSOR_FunctionEnter();

    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
    }
    else
    {
        // Reset the pedometer
        pSimulator->Reset();
    }

    SENSOR_FunctionExit(Status);
}

// Called by Sensor CLX to clear all history stored in the sensor.
NTSTATUS
PedometerDevice::OnClearHistory(
    _In_ SENSOROBJECT SensorInstance // Sensor device object
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;
    PHardwareSimulator pSimulator;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    if (FALSE == pDevice->m_HistorySupported)
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("PED %!FUNC! History is not supported by the HW");
        goto Exit;
    }
    
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);

    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Clear the pedometer history
    Status = pSimulator->ClearHistory();
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! Clear History failed %!STATUS!", Status);
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to start retrieving history.
//
// Arguments:
//      SensorInstance: IN: 
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
PedometerDevice::OnStartHistoryRetrieval(
    _In_ SENSOROBJECT SensorInstance,                                                 // sensor device object
    _Inout_updates_bytes_(HistorySizeInBytes) PSENSOR_COLLECTION_LIST pHistoryBuffer, // Pointer to a buffer containing the history elements
    _In_ ULONG HistorySizeInBytes                                                     // Size of the pHistoryBuffer buffer in bytes
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN IsLocked = FALSE;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    if (FALSE == pDevice->m_HistorySupported)
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("PED %!FUNC! History is not supported by the HW");
        goto Exit;
    }

    // Check if the previously created history retrieval thread finished.
    // When StartHistoryRetrieval is called again before we signal to CLX that history retrieval is
    // completed, fail the second call.
    WdfWaitLockAcquire(pDevice->m_Lock, NULL);
    BOOLEAN IsStarted = pDevice->m_HistoryRetrievalStarted;
    WdfWaitLockRelease(pDevice->m_Lock);

    if (FALSE != IsStarted)
    {
        Status = STATUS_DEVICE_BUSY;
        TraceError("PED %!FUNC! StartHistoryRetrieval called again before the previous call"
            "finishes retrieving history %!STATUS!", Status);
        goto Exit;
    }

    // check that the buffer is not null
    if (NULL == pHistoryBuffer)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! History buffer cannot be NULL. %!STATUS!", Status);
        goto Exit;
    }

    // check that the buffer can hold at least one record
    if (HistorySizeInBytes < pDevice->m_HistoryMarshalledRecordSize)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        TraceError("PED %!FUNC! History buffer is too small to even fill one complete entry. %!STATUS!", Status);
        goto Exit;
    }

    // Before creating a new thread, close handle to the previously created thread.
    if (NULL != pDevice->m_hThread)
    {
        CloseHandle(pDevice->m_hThread);
        pDevice->m_hThread = NULL;
    }

    pDevice->m_ClientHistoryBuffer = pHistoryBuffer;
    pDevice->m_ClientHistoryBufferSize = HistorySizeInBytes;

    WdfWaitLockAcquire(pDevice->m_Lock, NULL);
    IsLocked = TRUE;

    // Create a new thread to retrieve data from the driver's history buffer
    pDevice->m_hThread = CreateThread(NULL, // thread attributes
                                      0, // default stack size
                                      HistoryRetrievalThread, //start address
                                      reinterpret_cast<void*>(SensorInstance), // pointer to variable to be passed
                                      0, // run immediately
                                      NULL); // Thread ID
    if (NULL == pDevice->m_hThread)
    {
        Status = STATUS_UNSUCCESSFUL;
        TraceError("PED %!FUNC! CreateThread Failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice->m_HistoryRetrievalStarted = TRUE;
    WdfWaitLockRelease(pDevice->m_Lock);
    IsLocked = FALSE;

Exit:
    SENSOR_FunctionExit(Status);

    if (nullptr != pDevice)
    {
        if (FALSE != IsLocked)
        {
            WdfWaitLockRelease(pDevice->m_Lock);
        }

        if (!NT_SUCCESS(Status) && NULL != pDevice->m_hThread)
        {
            CloseHandle(pDevice->m_hThread);
            pDevice->m_hThread = NULL;
        }
    }

    return Status;
}



// Called by Sensor CLX to cancel history retrieval.
NTSTATUS
PedometerDevice::OnCancelHistoryRetrieval(
    _In_ SENSOROBJECT SensorInstance, // sensor device object
    _Out_ PULONG pBytesWritten // Upon exit, contains the number of bytes written to the history buffer
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;
    PHardwareSimulator pSimulator = nullptr;


    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pBytesWritten)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Invalid Parameters: Sensor(0x%p), pDevice (0x%p), pBytesWritten (0x%p). Failed %!STATUS!", SensorInstance, pDevice, pBytesWritten, Status);
        goto Exit;
    }

    if (FALSE == pDevice->m_HistorySupported)
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("PED %!FUNC! History is not supported by the HW");
        goto Exit;
    }

    *pBytesWritten = 0;

    // Check if history retrieval operation is in progress
    WdfWaitLockAcquire(pDevice->m_Lock, NULL);
    BOOLEAN IsStarted = pDevice->m_HistoryRetrievalStarted;
    WdfWaitLockRelease(pDevice->m_Lock);

    if (FALSE == IsStarted)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        TraceError("PED %!FUNC! History retrieval is not in progress %!STATUS!", Status);
        goto Exit;
    }

    // Get the simulator context
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
    }

    Status = pSimulator->SignalReadCancellation();
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! Failed to cancel history retrieval from HW %!STATUS!", Status);
    }

    // Wait for the history retrieval thread to finish writing the entry since no partial entries are allowed.
    DWORD result = WaitForSingleObjectEx(pDevice->m_hThread, Pedometer_TimeoutForHistoryThread_Ms, FALSE);
    if (WAIT_OBJECT_0 != result)
    {
        TraceError("PED %!FUNC! WaitForSingleObjectEx failed with error %d", result);
        Status = STATUS_DEVICE_BUSY;
        goto Exit;
    }

    *pBytesWritten = CollectionsListGetMarshalledSizeWithoutSerialization(pDevice->m_ClientHistoryBuffer);

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Created on calling StartHistoryRetrieval function
ULONG
WINAPI
PedometerDevice::HistoryRetrievalThread(
    _In_ LPVOID lpParam // sensor device object
    )
{
    WDF_OBJECT_ATTRIBUTES MemoryAttributes;
    WDFMEMORY MemoryHandle = NULL;
    PHardwareSimulator pSimulator = nullptr;
    SENSOROBJECT SensorInstance = reinterpret_cast<SENSOROBJECT>(lpParam);
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    ULONG ReturnCode = ERROR_SUCCESS;
    PPedometerSample HwHistoryBuffer = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG FillableCount = 0;

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. pDevice is null", SensorInstance);
        goto Exit;
    }

    // Get the simulator context
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Get the number of elements that can actually fit into the provided client buffer.
    FillableCount = (pDevice->m_ClientHistoryBufferSize - SENSOR_COLLECTION_LIST_HEADER_SIZE) / (pDevice->m_HistoryMarshalledRecordSize - SENSOR_COLLECTION_LIST_HEADER_SIZE);

    // Allocate enough memory to read the samples from HW
    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes,
        PagedPool,
        SENSOR_POOL_TAG_PEDOMETER,
        FillableCount * sizeof(PedometerSample),
        &MemoryHandle,
        reinterpret_cast<PVOID*>(&HwHistoryBuffer));
    if (!NT_SUCCESS(Status) || nullptr == HwHistoryBuffer)
    {
        TraceError("PED %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    Status = pSimulator->ReadHistory(&FillableCount, HwHistoryBuffer);
    if (STATUS_BUFFER_OVERFLOW == Status)
    {
        // It is okay to return as much as that fits in the buffer.
        TraceWarning("PED %!FUNC! Buffer Overflow. More entries available than requested %!STATUS!", Status);
    }
    else if (STATUS_CANCELLED == Status)
    {
        TraceWarning("PED %!FUNC! Retrieval canceled. Filling the buffer with retrieved entries %!STATUS!", Status);
    }
    else if (STATUS_NO_MORE_ENTRIES == Status)
    {
        TraceInformation("PED %!FUNC! No history entries available %!STATUS!", Status);
        goto Exit;
    }
    else if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! Failed to read history from the HW %!STATUS!", Status);
        goto Exit;
    }

    // All entries into the client's buffer must be complete i.e. no partial entries are allowed.
    for (ULONG index = 0; index < FillableCount; index++)
    {
        ULONG ElementIndex = 0;
        PedometerSample Data = HwHistoryBuffer[index];

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_TIMESTAMP;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_Timestamp;
        InitPropVariantFromFileTime(&(Data.Timestamp), &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_FIRST_AFTER_RESET;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerReset;
        InitPropVariantFromBoolean(Data.IsFirstAfterReset, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_UNKNOWN_STEP_TYPE;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepType;
        InitPropVariantFromUInt32(PedometerStepType_Unknown, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_UNKNOWN_STEP_COUNT;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepCount;
        InitPropVariantFromUInt32(Data.UnknownStepCount, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_UNKNOWN_STEP_DURATION;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepDuration_Ms;
        InitPropVariantFromInt64(Data.UnknownStepDurationMs, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_WALKING_STEP_TYPE;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepType;
        InitPropVariantFromUInt32(PedometerStepType_Walking, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_WALKING_STEP_COUNT;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepCount;
        InitPropVariantFromUInt32(Data.WalkingStepCount, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_WALKING_STEP_DURATION;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepDuration_Ms;
        InitPropVariantFromInt64(Data.WalkingStepDurationMs, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_RUNNING_STEP_TYPE;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepType;
        InitPropVariantFromUInt32(PedometerStepType_Running, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_RUNNING_STEP_COUNT;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepCount;
        InitPropVariantFromUInt32(Data.RunningStepCount, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        ElementIndex = (index * PEDOMETER_DATA_COUNT) + PEDOMETER_DATA_RUNNING_STEP_DURATION;
        pDevice->m_ClientHistoryBuffer->List[ElementIndex].Key = PKEY_SensorData_PedometerStepDuration_Ms;
        InitPropVariantFromInt64(Data.RunningStepDurationMs, &(pDevice->m_ClientHistoryBuffer->List[ElementIndex].Value));

        // Keep the count in the sensor collection list updated
        // so that the count is valid even when exit is signaled.
        pDevice->m_ClientHistoryBuffer->Count += PEDOMETER_DATA_COUNT;
    }

Exit:

    if (NULL != MemoryHandle)
    {
        // delete the memory handle and associated memory
        WdfObjectDelete(MemoryHandle);
    }

    // Update the state before we notify the Sensor CX as we are done with this request at this point
    // It would also help should the CX invoke 'EvtSensorStartHistoryRetrieval' on the same thread
    WdfWaitLockAcquire(pDevice->m_Lock, NULL);
    pDevice->m_HistoryRetrievalStarted = FALSE;
    WdfWaitLockRelease(pDevice->m_Lock);

    // call completed method only if retrieval was not canceled
    if (STATUS_CANCELLED != Status)
    {
        SensorsCxSensorHistoryRetrievalCompleted(pDevice->m_SensorInstance, CollectionsListGetMarshalledSizeWithoutSerialization(pDevice->m_ClientHistoryBuffer), Status);
    }
    
    return ReturnCode;
}



// Called by Sensor CLX to get supported data fields. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS
PedometerDevice::OnGetSupportedDataFields(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _Inout_opt_ PSENSOR_PROPERTY_LIST pFields, // Pointer to a list of supported properties
    _Out_ PULONG pSize // Number of bytes for the list of supported properties
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }

    if (nullptr == pFields)
    {
        // Just return size
        *pSize = pDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
    }
    else 
    {
        if (pFields->AllocatedSizeInBytes < pDevice->m_pSupportedDataFields->AllocatedSizeInBytes)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("PED %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out data
        Status = PropertiesListCopy(pFields, pDevice->m_pSupportedDataFields);
        if (!NT_SUCCESS(Status))
        {
            TraceError("PED %!FUNC! PropertiesListCopy failed %!STATUS!", Status);
            goto Exit;
        }

        *pSize = pDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
    }

Exit:
    if (!NT_SUCCESS(Status))
    {
        *pSize = 0;
    }
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to get sensor properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS
PedometerDevice::OnGetProperties(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // Pointer to a list of sensor properties
    _Out_ PULONG pSize // Number of bytes for the list of sensor properties
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }

    if (nullptr == pProperties)
    {
        // Just return size
        *pSize = CollectionsListGetMarshalledSize(pDevice->m_pProperties);
    }
    else 
    {
        if (pProperties->AllocatedSizeInBytes < 
            CollectionsListGetMarshalledSize(pDevice->m_pProperties))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("PED %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pProperties);
        if (!NT_SUCCESS(Status))
        {
            TraceError("PED %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
            goto Exit;
        }

        *pSize = CollectionsListGetMarshalledSize(pDevice->m_pProperties);
    }

Exit:
    if (!NT_SUCCESS(Status))
    {
        *pSize = 0;
    }
    SENSOR_FunctionExit(Status);
    return Status;
}


// Called by Sensor CLX to get data field properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS
PedometerDevice::OnGetDataFieldProperties(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _In_ const PROPERTYKEY *DataField, // Pointer to the propertykey of requested property
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // Pointer to a list of sensor properties
    _Out_ PULONG pSize // Number of bytes for the list of sensor properties
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize || nullptr == DataField)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }
    
    if ((*DataField == PKEY_SensorData_PedometerStepCount))
    {
        if (nullptr == pProperties)
        {
            // Just return size
            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pDataFieldProperties);
        }
        else 
        {
            if (pProperties->AllocatedSizeInBytes < 
                CollectionsListGetMarshalledSize(pDevice->m_pDataFieldProperties))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("PED %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
                goto Exit;
            }

            // Fill out all data
            Status = CollectionsListCopyAndMarshall (pProperties, pDevice->m_pDataFieldProperties);
            if (!NT_SUCCESS(Status))
            {
                TraceError("PED %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
                goto Exit;
            }

            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pDataFieldProperties);
        }
    }
    else
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("PED %!FUNC! Ped does NOT have properties for this data field. Failed %!STATUS!", Status);
        goto Exit;
    }

Exit:
    if (!NT_SUCCESS(Status))
    {
        *pSize = 0;
    }
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to get sampling rate of the sensor.
NTSTATUS
PedometerDevice::OnGetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _Out_ PULONG DataRateMs // Sampling rate in ms
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    if (nullptr == DataRateMs)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! DataRateMs(0x%p) parameter is invalid. Failed %!STATUS!", DataRateMs, Status);
        goto Exit;
    }

    *DataRateMs = pDevice->m_Interval;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to set sampling rate of the sensor.
NTSTATUS
PedometerDevice::OnSetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _In_ ULONG DataRateMs // Sampling rate in ms
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    if (Pedometer_Default_MinDataInterval_Ms > DataRateMs)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! DataRateMs(%d) parameter is smaller than the minimum data interval. Failed %!STATUS!", DataRateMs, Status);
        goto Exit;
    }

    pDevice->m_Interval = DataRateMs;

    // reschedule sample to return as soon as possible if it's started
    if (FALSE != pDevice->m_Started)
    {
        pDevice->m_Started = FALSE;
        WdfTimerStop(pDevice->m_Timer, TRUE);

        pDevice->m_Started = TRUE;
        pDevice->m_FirstSample = TRUE;
        WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(Pedometer_Default_MinDataInterval_Ms));
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to get data thresholds. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS
PedometerDevice::OnGetDataThresholds(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pThresholds, // Pointer to a list of sensor thresholds
    _Out_ PULONG pSize // Number of bytes for the list of sensor thresholds
    )
{
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }

    if (nullptr == pThresholds)
    {
        // Just return size
        *pSize = CollectionsListGetMarshalledSize(pDevice->m_pThresholds);
    }
    else
    {
        if (pThresholds->AllocatedSizeInBytes <
            CollectionsListGetMarshalledSize(pDevice->m_pThresholds))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("PED %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall(pThresholds, pDevice->m_pThresholds);
        if (!NT_SUCCESS(Status))
        {
            TraceError("PED %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
            goto Exit;
        }

        *pSize = CollectionsListGetMarshalledSize(pDevice->m_pThresholds);
    }

Exit:
    if (!NT_SUCCESS(Status))
    {
        *pSize = 0;
    }

    SENSOR_FunctionExit(Status);

    return Status;
}



// Called by Sensor CLX to set data thresholds.
NTSTATUS
PedometerDevice::OnSetDataThresholds(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _In_ PSENSOR_COLLECTION_LIST pThresholds // Pointer to a list of sensor thresholds
    )
{
    ULONG Element;
    BOOLEAN IsLocked = FALSE;
    PPedometerDevice pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (pDevice == nullptr)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    WdfWaitLockAcquire(pDevice->m_Lock, NULL);
    IsLocked = TRUE;

    for (Element = 0; Element < pThresholds->Count; Element++)
    {
        Status = PropKeyFindKeySetPropVariant(pDevice->m_pThresholds,
                                              &(pThresholds->List[Element].Key),
                                              TRUE,
                                              &(pThresholds->List[Element].Value));
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("PED %!FUNC! Pedometer driver does NOT have threshold for this data field. Failed %!STATUS!", Status);
            goto Exit;
        }
    }

    // Get data thresholds
    Status = PropKeyFindKeyGetUlong(pDevice->m_pThresholds,
                                    &PKEY_SensorData_PedometerStepCount,
                                    &(pDevice->m_CachedThreshold));
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! PropKeyFindKeyGetUlong for PedometerStepCount failed! %!STATUS!", Status);
        goto Exit;
    }

Exit:
    if (FALSE != IsLocked)
    {
        WdfWaitLockRelease(pDevice->m_Lock);
        IsLocked = FALSE;
    }
    SENSOR_FunctionExit(Status);
    return Status;
}


// Called by Sensor CLX to handle IOCTLs that clx does not support
NTSTATUS
PedometerDevice::OnIoControl(
    _In_ SENSOROBJECT /*SensorInstance*/, // WDF queue object
    _In_ WDFREQUEST /*Request*/,          // WDF request object
    _In_ size_t /*OutputBufferLength*/,   // number of bytes to retrieve from output buffer
    _In_ size_t /*InputBufferLength*/,    // number of bytes to retrieve from input buffer
    _In_ ULONG /*IoControlCode*/          // IOCTL control code
    )
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}