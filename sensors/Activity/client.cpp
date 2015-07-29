// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//     This module contains the implementation of driver callback function from clx to activity detection driver.
//
// Environment:
//     Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "HardwareSimulator.h"
#include "Client.tmh"

// This routine is called by worker thread to read a single sample, compare threshold
// and push it back to CLX. It simulates hardware thresholding by only generating data
// when the change of data is greater than threshold.
NTSTATUS ActivityDevice::GetData()
{
    BOOLEAN dataReady = FALSE;
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (NULL != m_SimulatorInstance)
    {
        // Use simulator to get m_pFiltered Sample
        PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(m_SimulatorInstance);
        if (nullptr != pSimulator)
        {
            status = pSimulator->GetSample(m_pFilteredSample);
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    
    if (NT_SUCCESS(status))
    {
        status = CollectionsListSortSubscribedActivitiesByConfidence(m_pThresholds, m_pFilteredSample);
        if (!NT_SUCCESS(status))
        {
            TraceError("ACT %!FUNC! CollectionsListSortSubscribedActivitiesByConfidence failed! %!STATUS!", status);
        }
        else
        {
            // new sample?
            if (FALSE != m_FirstSample)
            {
                dataReady = TRUE;
            }
            else
            {
                dataReady = EvaluateActivityThresholds(m_pFilteredSample, m_pLastSample, m_pThresholds);
            }
        }
    }

    if (FALSE != dataReady)
    {
        // update last sample
        FILETIME TimeStamp = {};
        memcpy_s(m_pLastSample, m_pFilteredSample->AllocatedSizeInBytes, m_pFilteredSample, m_pFilteredSample->AllocatedSizeInBytes);
        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pLastSample->List[ACTIVITY_DATA_TIMESTAMP].Value));

        // push to clx
        SensorsCxSensorDataReady(m_SensorInstance, m_pLastSample);
        m_FirstSample = FALSE;
    }
    else
    {
        status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("ACT %!FUNC! Data did NOT meet the threshold");
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This callback is called when interval wait time has expired and driver is ready
// to collect new sample. The callback reads current value, compare value to threshold,
// pushes it up to CLX framework, and schedule next wake up time.
VOID ActivityDevice::OnTimerExpire(_In_ WDFTIMER timer)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(WdfTimerGetParentObject(timer));
    if (nullptr == pDevice)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("ACT %!FUNC! GetActivityContextFromSensorInstance failed %!STATUS!", status);
    }
    else
    {
        // Get data and push to clx
        WdfWaitLockAcquire(pDevice->m_Lock, NULL);
        status = pDevice->GetData();
        if (!NT_SUCCESS(status) && STATUS_DATA_NOT_ACCEPTED != status)
        {
            TraceError("ACT %!FUNC! GetAccData Failed %!STATUS!", status);
        }
        WdfWaitLockRelease(pDevice->m_Lock);

        // Schedule next wake up time
        if (Act_Default_MinDataInterval_Ms <= pDevice->m_Interval &&
            FALSE != pDevice->m_PoweredOn &&
            FALSE != pDevice->m_Started)
        {
            WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(pDevice->m_Interval));
        }
    }

    SENSOR_FunctionExit(status);
}

// Called by Sensor CLX to begin continously sampling the sensor.
NTSTATUS ActivityDevice::OnStart(_In_ SENSOROBJECT sensorInstance)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        if (FALSE == pDevice->m_PoweredOn)
        {
            status = STATUS_DEVICE_NOT_READY;
            TraceError("ACT %!FUNC! Sensor is not powered on! %!STATUS!", status);
        }
        else
        {
            // Start simulation
            if (NULL != pDevice->m_SimulatorInstance)
            {
                PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
                if (nullptr != pSimulator)
                {
                    pSimulator->Start();
                }
            }

            // Start sensing
            pDevice->m_FirstSample = TRUE;
            pDevice->m_Started = TRUE;
            InitPropVariantFromUInt32(SensorState_Active, &(pDevice->m_pProperties->List[SENSOR_PROPERTY_STATE].Value));
            WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(Act_Default_MinDataInterval_Ms));
        }
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to stop continously sampling the sensor.
NTSTATUS ActivityDevice::OnStop(_In_ SENSOROBJECT sensorInstance)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();
    
    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        // Stop sensing
        pDevice->m_Started = FALSE;
        WdfTimerStop(pDevice->m_Timer, TRUE);
        InitPropVariantFromUInt32(SensorState_Idle, &(pDevice->m_pProperties->List[SENSOR_PROPERTY_STATE].Value));
    
        // Stop simulation
        if (NULL != pDevice->m_SimulatorInstance)
        {
            PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
            if (nullptr != pSimulator)
            {
                pSimulator->Stop();
            }
        }
    }
    
    SENSOR_FunctionExit(status);
    return status;
}

// This routine is called by history retrieval thread to remove an entry from the history buffer.
// Note this function must be called under lock
_Requires_lock_held_(m_HistoryLock)
NTSTATUS ActivityDevice::RemoveDataElementFromHistoryBuffer(_Out_ PActivitySample pData)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (nullptr == pData)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! NULL ptr passed in %!STATUS!", status);
    }
    else
    {
        if (0 == m_History.NumOfElems)
        {
            // buffer empty
            status = STATUS_NO_MORE_ENTRIES;
        }
        else
        {
            // Remove the oldest element from the circular buffer
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
    }

    return status;
}

// This routine is called by worker thread to add an entry to the history buffer.
// Note this function must be called under lock
_Requires_lock_held_(m_HistoryLock)
NTSTATUS ActivityDevice::AddDataElementToHistoryBuffer(_In_ PActivitySample pData)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (nullptr == pData)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! GetActivityContextFromSensorInstance failed %!STATUS!", status);
    }
    else
    {
        if (0 == m_History.NumOfElems)
        {
            // buffer empty
            m_History.NumOfElems++;
        }
        else if (m_History.BufferLength > m_History.NumOfElems)
        {
            // buffer not full yet. Increment the index of the last element
            m_History.LastElemIndex++;
            m_History.LastElemIndex %= m_History.BufferLength;
            m_History.NumOfElems++;
        }
        else if (m_History.BufferLength == m_History.NumOfElems)
        {
            // buffer full. Over-write the oldest element
            m_History.FirstElemIndex++;
            m_History.FirstElemIndex %= m_History.BufferLength;
            m_History.LastElemIndex++;
            m_History.LastElemIndex %= m_History.BufferLength;
        }
        memcpy_s(&(m_History.pData[m_History.LastElemIndex]),
            sizeof(m_History.pData[m_History.LastElemIndex]),
            pData,
            sizeof(*pData));
    }

    return status;
}

// This routine is for clearing the history buffer. Note this function must be called under lock.
_Requires_lock_held_(m_HistoryLock)
NTSTATUS ActivityDevice::ClearHistoryBuffer()
{
    m_History.FirstElemIndex = 0;
    m_History.LastElemIndex = 0;
    m_History.NumOfElems = 0;
    RtlZeroMemory(m_History.pData, (m_History.BufferLength*sizeof(ActivitySample)));
    return STATUS_SUCCESS;
}

// This callback is called when interval wait time has expired and driver is ready
// to collect new sample. The callback stores activity data in history buffer,
// and schedules next wake up time.
VOID ActivityDevice::OnHistoryTimerExpire(_In_ WDFTIMER historyTimer)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(WdfTimerGetParentObject(historyTimer));
    if (nullptr == pDevice)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("ACT %!FUNC! GetActivityContextFromSensorInstance failed %!STATUS!", status);
    }
    else
    {
        ActivitySample data = {};
        if (NULL != pDevice->m_SimulatorInstance)
        {
            PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
            if (nullptr != pSimulator)
            {
                status = pSimulator->GetSample(&data);
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        GetSystemTimePreciseAsFileTime(&(data.Timestamp));

        if (NT_SUCCESS(status))
        {
            // Add data to the buffer
            WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);
            status = pDevice->AddDataElementToHistoryBuffer(&data);
            if (!NT_SUCCESS(status))
            {
                TraceError("ACT %!FUNC! AddDataElementToHistoryBuffer Failed %!STATUS!", status);
            }
            WdfWaitLockRelease(pDevice->m_HistoryLock);
        }

        // Schedule next wake up time
        if (FALSE != pDevice->m_HistoryStarted)
        {
            WdfTimerStart(pDevice->m_HistoryTimer, WDF_REL_TIMEOUT_IN_MS(pDevice->m_HistoryIntervalInMs));
        }
    }

    SENSOR_FunctionExit(status);
}

// Called by Sensor CLX to begin keeping history
NTSTATUS ActivityDevice::OnStartHistory(_In_ SENSOROBJECT sensorInstance)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else if (0 == pDevice->m_HistorySizeInRecords)
    {
        status = STATUS_NOT_SUPPORTED;
        TraceError("ACT %!FUNC! Sensor does not support History");
    }
    else if (FALSE == pDevice->m_PoweredOn)
    {
        status = STATUS_DEVICE_NOT_READY;
        TraceError("ACT %!FUNC! Sensor is not powered on! %!STATUS!", status);
    }
    else
    {
        // Start keeping history
        pDevice->m_HistoryStarted = TRUE;
        WdfTimerStart(pDevice->m_HistoryTimer, WDF_REL_TIMEOUT_IN_MS(pDevice->m_HistoryIntervalInMs));
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to stop keeping history.
NTSTATUS ActivityDevice::OnStopHistory(_In_ SENSOROBJECT sensorInstance)
{ 
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        // Stop keeping history
        pDevice->m_HistoryStarted = FALSE;
        WdfTimerStop(pDevice->m_HistoryTimer, TRUE);
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to clear all history stored in the sensor.
NTSTATUS ActivityDevice::OnClearHistory(_In_ SENSOROBJECT sensorInstance)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);
        status = pDevice->ClearHistoryBuffer();
        if (!NT_SUCCESS(status))
        {
            TraceError("ACT %!FUNC! ClearHistoryBuffer Failed %!STATUS!", status);
        }
        WdfWaitLockRelease(pDevice->m_HistoryLock);
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to start retrieving history.
NTSTATUS ActivityDevice::OnStartHistoryRetrieval(
    _In_ SENSOROBJECT sensorInstance,
    _Inout_updates_bytes_(historySizeInBytes) PSENSOR_COLLECTION_LIST pHistoryBuffer,
    _In_ ULONG historySizeInBytes
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        // Check if the previously created history retrieval thread finished.
        // When StartHistoryRetrieval is called again before we signal to CLX that history retrieval is
        // completed, fail the second call.
        WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);
        BOOLEAN IsStarted = pDevice->m_HistoryRetrievalStarted;
        WdfWaitLockRelease(pDevice->m_HistoryLock);

        if (FALSE != IsStarted)
        {
            status = STATUS_DEVICE_BUSY;
            TraceError("ACT %!FUNC! StartHistoryRetrieval called again before the previous call"
                "finishes retrieving history %!STATUS!", status);
        }
        else
        {
            // Before creating a new thread, close handle to the previously created thread.
            if (NULL != pDevice->m_hThread)
            {
                CloseHandle(pDevice->m_hThread);
                pDevice->m_hThread = NULL;
            }

            // check that the buffer is not null
            if (NULL == pHistoryBuffer)
            {
                status = STATUS_INVALID_PARAMETER;
                TraceError("ACT %!FUNC! History buffer cannot be NULL. %!STATUS!", status);
            }
            else
            {
                // check that the buffer can hold at least one entry
                if (historySizeInBytes < pDevice->m_HistoryMarshalledRecordSize)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    TraceError("ACT %!FUNC! History buffer is too small to even fill one complete entry. %!STATUS!", status);
                }
                else
                {
                    pDevice->m_ClientHistoryBuffer = pHistoryBuffer;
                    pDevice->m_ClientHistoryBufferSize = historySizeInBytes;

                    // Reset exit event
                    ResetEvent(pDevice->m_ExitEvent);

                    WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);

                    // Create a new thread to retrieve data from the driver's history buffer
                    pDevice->m_hThread = CreateThread(NULL,         // thread attributes
                        0,                                          // default stack size
                        HistoryRetrievalThread,                     //start address
                        reinterpret_cast<PVOID>(sensorInstance),    // pointer to variable to be passed
                        0,                                          // run immediately
                        NULL);                                      // Thread ID
                    if (NULL == pDevice->m_hThread)
                    {
                        status = STATUS_UNSUCCESSFUL;
                        TraceError("ACT %!FUNC! CreateThread Failed %!STATUS!", status);
                    }
                    else
                    {
                        pDevice->m_HistoryRetrievalStarted = TRUE;
                    }

                    WdfWaitLockRelease(pDevice->m_HistoryLock);
                }
            }
        }
    }

    SENSOR_FunctionExit(status);

    if (nullptr != pDevice && !NT_SUCCESS(status) && NULL != pDevice->m_hThread)
    {
        CloseHandle(pDevice->m_hThread);
        pDevice->m_hThread = NULL;
    }
    return status;
}

// Called by Sensor CLX to cancel history retrieval.
NTSTATUS ActivityDevice::OnCancelHistoryRetrieval(_In_ SENSOROBJECT sensorInstance, _Out_ PULONG pBytesWritten)
{
    NTSTATUS status = STATUS_SUCCESS;
    *pBytesWritten = 0;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        // Check if history retrieval operation is in progress
        WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);
        BOOLEAN IsStarted = pDevice->m_HistoryRetrievalStarted;
        WdfWaitLockRelease(pDevice->m_HistoryLock);

        if (FALSE == IsStarted)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            TraceError("ACT %!FUNC! History retrieval is not in progress %!STATUS!", status);
        }
        else
        {
            // Signal the history retrieval thread to exit
            SetEvent(pDevice->m_ExitEvent);

            // Wait for the history retrieval thread to finish writing the entry since no partial entries are allowed.
            DWORD result = WaitForSingleObjectEx(pDevice->m_hThread, Act_TimeoutForHistoryThread_Ms, FALSE);
            if (WAIT_OBJECT_0 != result)
            {
                // continue to mark history retrieval stop
                status = NTSTATUS_FROM_WIN32(result);
                TraceError("ACT %!FUNC! WaitForSingleObjectEx failed! %!STATUS!", status);
            }

            WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);
            *pBytesWritten = CollectionsListGetMarshalledSizeWithoutSerialization(pDevice->m_ClientHistoryBuffer);
            pDevice->m_HistoryRetrievalStarted = FALSE;
            WdfWaitLockRelease(pDevice->m_HistoryLock);
        }
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Created on calling StartHistoryRetrieval function.
ULONG WINAPI ActivityDevice::HistoryRetrievalThread(_In_ LPVOID lpParam)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN exitSignal = FALSE;
    UINT existingCount = 0;
    UINT fillableCount = 0;
    
    SENSOROBJECT sensorInstance = reinterpret_cast<SENSOROBJECT>(lpParam);
    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice ||
        NULL == pDevice->m_ClientHistoryBuffer ||
        pDevice->m_HistoryMarshalledRecordSize > pDevice->m_ClientHistoryBufferSize)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        // Capture the number of entries in the driver's history buffer at this moment
        WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);
        existingCount = pDevice->m_History.NumOfElems;
        WdfWaitLockRelease(pDevice->m_HistoryLock);

        // How many entries can the client supplied buffer hold
        fillableCount = (pDevice->m_ClientHistoryBufferSize - SENSOR_COLLECTION_LIST_HEADER_SIZE) / (pDevice->m_HistoryMarshalledRecordSize - SENSOR_COLLECTION_LIST_HEADER_SIZE);

        // Find the minimum of the number of entries in the driver's history buffer and 
        // the number of entries that can be held by the client supplied buffer
        UINT count = min(existingCount, fillableCount);

        // All entries into the client's buffer must be complete i.e. no partial entries are allowed.
        // Check whether the exit event is signaled before looping through each time to retrieve an entry from the driver's history buffer.
        // Each history record will only contain first entry of the most probable state.
        for (UINT index = 0; index < count; index++)
        {
            ActivitySample data = {};
            UINT elementIndex = ACTIVITY_DATA_COUNT * index;
            if (WAIT_OBJECT_0 == WaitForSingleObject(pDevice->m_ExitEvent, 0))
            {
                exitSignal = TRUE;
                break;
            }

            WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);
            if (!NT_SUCCESS(pDevice->RemoveDataElementFromHistoryBuffer(&data)))
            {
                // Only return error if there is no more entry which is not an error. Therefore no need to set status.
                WdfWaitLockRelease(pDevice->m_HistoryLock);
                break;
            }

            elementIndex = (index * ACTIVITY_DATA_COUNT) + ACTIVITY_DATA_TIMESTAMP;
            pDevice->m_ClientHistoryBuffer->List[elementIndex].Key = PKEY_SensorData_Timestamp;
            InitPropVariantFromFileTime(&(data.Timestamp), &(pDevice->m_ClientHistoryBuffer->List[elementIndex].Value));

            elementIndex = (index * ACTIVITY_DATA_COUNT) + ACTIVITY_DATA_CURRENT_STATE;
            pDevice->m_ClientHistoryBuffer->List[elementIndex].Key = PKEY_SensorData_CurrentActivityState;
            InitPropVariantFromUInt32(data.Activity, &(pDevice->m_ClientHistoryBuffer->List[elementIndex].Value));

            elementIndex = (index * ACTIVITY_DATA_COUNT) + ACTIVITY_DATA_CURRENT_CONFIDENCE;
            pDevice->m_ClientHistoryBuffer->List[elementIndex].Key = PKEY_SensorData_CurrentActivityStateConfidence_Percentage;
            InitPropVariantFromUInt16(data.Confidence, &(pDevice->m_ClientHistoryBuffer->List[elementIndex].Value));

            // Keep the count in the sensor collection list updated
            // so that the count is valid even when exit is signaled.
            pDevice->m_ClientHistoryBuffer->Count = (index + 1) * ACTIVITY_DATA_COUNT;
            WdfWaitLockRelease(pDevice->m_HistoryLock);
        }
    }

    if (FALSE == exitSignal)
    {
        if (fillableCount < existingCount)
        {
            status = STATUS_BUFFER_OVERFLOW;
        }
        else if (0 == existingCount)
        {
            status = STATUS_NO_MORE_ENTRIES;
        }
        // call completed callback only if retrieval was not canceled
        SensorsCxSensorHistoryRetrievalCompleted(pDevice->m_SensorInstance, CollectionsListGetMarshalledSizeWithoutSerialization(pDevice->m_ClientHistoryBuffer), status);
    }

    WdfWaitLockAcquire(pDevice->m_HistoryLock, NULL);
    pDevice->m_HistoryRetrievalStarted = FALSE;
    WdfWaitLockRelease(pDevice->m_HistoryLock);

    return ERROR_SUCCESS;
}

// Called by Sensor CLX to get supported data fields. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS ActivityDevice::OnGetSupportedDataFields(
    _In_ SENSOROBJECT sensorInstance,           // sensor device object
    _Inout_opt_ PSENSOR_PROPERTY_LIST pFields,  // pointer to a list of supported properties
    _Out_ PULONG pSize)                         // number of bytes for the list of supported properties
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice || nullptr == pSize)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Invalid parameters! %!STATUS!", status);
    }
    else
    {
        if (nullptr == pFields)
        {
            // Just return size
            *pSize = pDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
        }
        else
        {
            if (pFields->AllocatedSizeInBytes < pDevice->m_pSupportedDataFields->AllocatedSizeInBytes)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("ACT %!FUNC! Buffer is too small. Failed %!STATUS!", status);
            }
            else
            {
                // Fill out data
                status = PropertiesListCopy(pFields, pDevice->m_pSupportedDataFields);
                if (!NT_SUCCESS(status))
                {
                    TraceError("ACT %!FUNC! PropertiesListCopy failed %!STATUS!", status);
                }
                else
                {
                    *pSize = pDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
                }
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        *pSize = 0;
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to get sensor properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS ActivityDevice::OnGetProperties(
    _In_ SENSOROBJECT sensorInstance,                   // sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties,    // pointer to a list of sensor properties
    _Out_ PULONG pSize)                                 // number of bytes for the list of sensor properties
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice || nullptr == pSize)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Invalid parameters! %!STATUS!", status);
    }
    else
    {
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
                status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("ACT %!FUNC! Buffer is too small. Failed %!STATUS!", status);
            }
            else
            {
                // Fill out all data
                status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pProperties);
                if (!NT_SUCCESS(status))
                {
                    TraceError("ACT %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", status);
                }
                else
                {
                    *pSize = CollectionsListGetMarshalledSize(pDevice->m_pProperties);
                }
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        *pSize = 0;
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to get data field properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS ActivityDevice::OnGetDataFieldProperties(
    _In_ SENSOROBJECT sensorInstance,                   // sensor device object
    _In_ const PROPERTYKEY *pDataField,                 // pointer to the propertykey of requested property
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties,    // pointer to a list of sensor properties
    _Out_ PULONG pSize)                                 // number of bytes for the list of sensor properties
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice || nullptr == pSize || nullptr == pDataField)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Invalid parameters! %!STATUS!", status);
    }
    else
    {
        if (*pDataField == PKEY_SensorData_CurrentActivityStateConfidence_Percentage)
        {
            if (nullptr == pProperties)
            {
                // Just return size
                *pSize = CollectionsListGetMarshalledSize(pDevice->m_pDataFieldProperties);
            }
            else
            {
                if (pProperties->AllocatedSizeInBytes < CollectionsListGetMarshalledSize(pDevice->m_pDataFieldProperties))
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    TraceError("ACT %!FUNC! Buffer is too small. Failed %!STATUS!", status);
                }
                else
                {
                    // Fill out all data
                    status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pDataFieldProperties);
                    if (!NT_SUCCESS(status))
                    {
                        TraceError("ACT %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", status);
                    }
                    else
                    {
                        *pSize = CollectionsListGetMarshalledSize(pDevice->m_pDataFieldProperties);
                    }
                }
            }
        }
        else
        {
            status = STATUS_NOT_SUPPORTED;
            TraceError("ACT %!FUNC! AccFake does NOT have properties for this data field. Failed %!STATUS!", status);
        }
    }

    if (!NT_SUCCESS(status))
    {
        *pSize = 0;
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to get sampling rate of the sensor.
NTSTATUS ActivityDevice::OnGetDataInterval(_In_ SENSOROBJECT sensorInstance, _Out_ PULONG pDataRateMs)
{
    NTSTATUS status = STATUS_SUCCESS;
    *pDataRateMs = 0;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice || nullptr == pDataRateMs)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        *pDataRateMs = pDevice->m_Interval;
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to set sampling rate of the sensor.
NTSTATUS ActivityDevice::OnSetDataInterval(_In_ SENSOROBJECT sensorInstance, _In_ ULONG dataRateMs)
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice || Act_Default_MinDataInterval_Ms > dataRateMs)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor parameter is invalid. Failed %!STATUS!", status);
    }
    else
    {
        pDevice->m_Interval = dataRateMs;

        // reschedule sample to return as soon as possible if it's started
        if (FALSE != pDevice->m_Started)
        {
            pDevice->m_Started = FALSE;
            WdfTimerStop(pDevice->m_Timer, TRUE);

            pDevice->m_Started = TRUE;
            pDevice->m_FirstSample = TRUE;
            WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(Act_Default_MinDataInterval_Ms));
        }
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to get data thresholds. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS ActivityDevice::OnGetDataThresholds(
    _In_ SENSOROBJECT sensorInstance,                   // sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pThresholds,    // pointer to a list of sensor thresholds
    _Out_ PULONG pSize)                                 // number of bytes for the list of sensor thresholds
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice || nullptr == pSize)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Invalid parameters! %!STATUS!", status);
    }
    else
    {
        if (nullptr == pThresholds)
        {
            // Just return size
            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pThresholds);
        }
        else
        {
            if (pThresholds->AllocatedSizeInBytes < CollectionsListGetMarshalledSize(pDevice->m_pThresholds))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("ACT %!FUNC! Buffer is too small. Failed %!STATUS!", status);
            }
            else
            {
                // Fill out all data
                status = CollectionsListCopyAndMarshall(pThresholds, pDevice->m_pThresholds);
                if (!NT_SUCCESS(status))
                {
                    TraceError("ACT %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", status);
                }
                else
                {
                    *pSize = CollectionsListGetMarshalledSize(pDevice->m_pThresholds);
                }
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        *pSize = 0;
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to set data thresholds.
NTSTATUS ActivityDevice::OnSetDataThresholds(
    _In_ SENSOROBJECT sensorInstance,           // sensor device object
    _In_ PSENSOR_COLLECTION_LIST pThresholds)   // pointer to a list of sensor thresholds
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
    if (nullptr == pDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT) sensorInstance, status);
    }
    else
    {
        WdfWaitLockAcquire(pDevice->m_Lock, NULL);

        for (ULONG element = 0; element < pThresholds->Count; element++)
        {
            status = PropKeyFindKeySetPropVariant(pDevice->m_pThresholds,
                &(pThresholds->List[element].Key),
                TRUE,
                &(pThresholds->List[element].Value));
            if (!NT_SUCCESS(status))
            {
                status = STATUS_INVALID_PARAMETER;
                TraceError("ACT %!FUNC! ActivityFake does NOT have threshold for this data field. Failed %!STATUS!", status);
                break;
            }
        }

        WdfWaitLockRelease(pDevice->m_Lock);
    }

    SENSOR_FunctionExit(status);
    return status;
}

// Called by Sensor CLX to handle IOCTLs that clx does not support.
NTSTATUS ActivityDevice::OnIoControl(
    _In_ SENSOROBJECT /*sensorInstance*/,   // WDF queue object
    _In_ WDFREQUEST request,                // WDF request object
    _In_ size_t /*outputBufferLength*/,     // number of bytes to retrieve from output buffer
    _In_ size_t /*inputBufferLength*/,      // number of bytes to retrieve from input buffer
    _In_ ULONG /*ioControlCode*/)           // IOCTL control code
{
    SENSOR_FunctionEnter();

    WdfRequestComplete(request, STATUS_NOT_SUPPORTED);

    SENSOR_FunctionExit(STATUS_NOT_SUPPORTED);
    return STATUS_NOT_SUPPORTED;
}
