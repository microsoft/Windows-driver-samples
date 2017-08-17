// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//
//  This module contains the implementation of driver callback function
//  from clx to clients.
//
// Environment:
//
//  Windows User-Mode Driver Framework (WUDF)

#include "Clients.h"

#include "Clients.tmh"

static const UINT SYSTEM_TICK_COUNT_1MS = 1; // 1ms
//------------------------------------------------------------------------------
// Function: OnTimerExpire
//
// This callback is called when interval wait time has expired and driver is ready
// to collect new sample. The callback reads current value, compare value to threshold,
// pushes it up to CLX framework, and schedule next wake up time.
//
// Arguments:
//      Timer: IN: WDF timer object
//
// Return Value:
//      None
//------------------------------------------------------------------------------
VOID
OnTimerExpire(
    _In_ WDFTIMER Timer
    )
{
    PComboDevice pDevice = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    pDevice = GetContextFromSensorInstance(WdfTimerGetParentObject(Timer));
    if (pDevice == nullptr)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("COMBO %!FUNC! GetContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Get data and push to clx
    Lock(pDevice->m_Lock);
    Status = pDevice->GetData();
    if (!NT_SUCCESS(Status) && Status != STATUS_DATA_NOT_ACCEPTED)
    {
        TraceError("COMBO %!FUNC! GetData Failed %!STATUS!", Status);
    }
    Unlock(pDevice->m_Lock);

    // Schedule next wake up time
    if (pDevice->m_MinimumIntervalMs <= pDevice->m_IntervalMs &&
        FALSE != pDevice->m_PoweredOn &&
        FALSE != pDevice->m_Started)
    {
        LONGLONG WaitTime = 0;  // in unit of 100ns

        if (pDevice->m_StartTime == 0)
        {
            // in case we fail to get sensor start time, use static wait time
            WaitTime = WDF_REL_TIMEOUT_IN_MS(pDevice->m_IntervalMs);
        }
        else
        {
            ULONG CurrentTimeMs = 0;

            // dynamically calculate wait time to avoid jitter
            Status = GetPerformanceTime (&CurrentTimeMs);
            if (!NT_SUCCESS(Status))
            {
                TraceError("COMBO %!FUNC! GetPerformanceTime %!STATUS!", Status);
                WaitTime = WDF_REL_TIMEOUT_IN_MS(pDevice->m_IntervalMs);
            }
            else
            {
                pDevice->m_SampleCount++;
                if (CurrentTimeMs > (pDevice->m_StartTime + (pDevice->m_IntervalMs * (pDevice->m_SampleCount + 1))))
                {
                    // If we skipped two or more beats, reschedule the timer with a zero due time to catch up on missing samples
                    WaitTime = 0;
                }
                else
                {
                    // Else, just compute the remaining time
                    WaitTime = (pDevice->m_StartTime +
                        (pDevice->m_IntervalMs * (pDevice->m_SampleCount + 1))) - CurrentTimeMs;
                }

                WaitTime = WDF_REL_TIMEOUT_IN_MS(WaitTime);
            }
        }

        WdfTimerStart(pDevice->m_Timer, WaitTime);
    }

Exit:

    SENSOR_FunctionExit(Status);
}



//------------------------------------------------------------------------------
// Function: OnStart
//
// Called by Sensor CLX to begin continously sampling the sensor.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnStart(
    _In_ SENSOROBJECT SensorInstance
    )
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (pDevice == nullptr)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Sensor(%p) parameter is invalid. Failed %!STATUS!", static_cast<PVOID>(&SensorInstance), Status);
        goto Exit;
    }

    if (pDevice->m_PoweredOn == FALSE)
    {
        Status = STATUS_DEVICE_NOT_READY;
        TraceError("COMBO %!FUNC! Sensor is not powered on! %!STATUS!", Status);
        goto Exit;
    }

    //
    // Start worker thread
    //

    pDevice->m_IntervalMs;

    pDevice->m_FirstSample = TRUE;

    // Start polling

    pDevice->m_Started = TRUE;

    InitPropVariantFromUInt32(SensorState_Active,
                              &(pDevice->m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

    WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(pDevice->m_MinimumIntervalMs));

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



//------------------------------------------------------------------------------
// Function: OnStop
//
// Called by Sensor CLX to stop continously sampling the sensor.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnStop(
    _In_ SENSOROBJECT SensorInstance
    )
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (pDevice == nullptr)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Sensor(%p) parameter is invalid. Failed %!STATUS!", static_cast<PVOID>(&SensorInstance), Status);
        goto Exit;
    }

    // Stop polling

    pDevice->m_Started = FALSE;

    WdfTimerStop(pDevice->m_Timer, TRUE);

    InitPropVariantFromUInt32(SensorState_Idle,
                              &(pDevice->m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

    //
    // Restoring system time resolution
    //
    if (TIMERR_NOERROR != timeEndPeriod(SYSTEM_TICK_COUNT_1MS))
    {
        // not a failure, just log message
        Status = STATUS_UNSUCCESSFUL;
        TraceWarning("COMBO %!FUNC! timeEndPeriod failed to restore timer resolution! %!STATUS!", Status);
    }

Exit:
    SENSOR_FunctionExit(Status);

    return Status;
}



//------------------------------------------------------------------------------
// Function: OnGetSupportedDataFields
//
// Called by Sensor CLX to get supported data fields. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size
// for the buffer, allocate buffer, then call the function again to retrieve
// sensor information.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//      pFields: INOUT_OPT: pointer to a list of supported properties
//      pSize: OUT: number of bytes for the list of supported properties
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnGetSupportedDataFields(
    _In_ SENSOROBJECT SensorInstance,
    _Inout_opt_ PSENSOR_PROPERTY_LIST pFields,
    _Out_ PULONG pSize
    )
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Invalid parameters! %!STATUS!", Status);
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
            TraceError("COMBO %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out data
        Status = PropertiesListCopy (pFields, pDevice->m_pSupportedDataFields);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! PropertiesListCopy failed %!STATUS!", Status);
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



//------------------------------------------------------------------------------
// Function: OnGetProperties
//
// Called by Sensor CLX to get sensor properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size
// for the buffer, allocate buffer, then call the function again to retrieve
// sensor information.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//      pProperties: INOUT_OPT: pointer to a list of sensor properties
//      pSize: OUT: number of bytes for the list of sensor properties
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnGetProperties(
    _In_ SENSOROBJECT SensorInstance,
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties,
    _Out_ PULONG pSize
    )
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Invalid parameters! %!STATUS!", Status);
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
            TraceError("COMBO %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pProperties);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
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


//------------------------------------------------------------------------------
// Function: OnGetDataFieldProperties
//
// Called by Sensor CLX to get data field properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size
// for the buffer, allocate buffer, then call the function again to retrieve
// sensor information.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//      DataField: IN: pointer to the propertykey of requested property
//      pProperties: INOUT_OPT: pointer to a list of sensor properties
//      pSize: OUT: number of bytes for the list of sensor properties
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnGetDataFieldProperties(
    _In_ SENSOROBJECT SensorInstance,
    _In_ const PROPERTYKEY *DataField,
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties,
    _Out_ PULONG pSize
)
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize || nullptr == DataField)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }

    if (IsKeyPresentInPropertyList(pDevice->m_pSupportedDataFields, DataField) != FALSE)
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
                TraceError("COMBO %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
                goto Exit;
            }

            // Fill out all data
            Status = CollectionsListCopyAndMarshall (pProperties, pDevice->m_pDataFieldProperties);
            if (!NT_SUCCESS(Status))
            {
                TraceError("COMBO %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
                goto Exit;
            }

            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pDataFieldProperties);
        }
    }
    else
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("COMBO %!FUNC! Sensor does NOT have properties for this data field. Failed %!STATUS!", Status);
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



//------------------------------------------------------------------------------
// Function: OnGetDataInterval
//
// Called by Sensor CLX to get sampling rate of the sensor.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//      DataRateMs: OUT: sampling rate in ms
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnGetDataInterval(
    _In_ SENSOROBJECT SensorInstance,
    _Out_ PULONG DataRateMs
    )
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (pDevice == nullptr)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Invalid parameter!");
        goto Exit;
    }

    if (DataRateMs == nullptr)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! DataRateMs(%p) parameter is invalid. Failed %!STATUS!", static_cast<PVOID>(DataRateMs), Status);
        goto Exit;
    }

    *DataRateMs = pDevice->m_IntervalMs;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



//------------------------------------------------------------------------------
// Function: OnSetDataInterval
//
// Called by Sensor CLX to set sampling rate of the sensor.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//      DataRateMs: IN: sampling rate in ms
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnSetDataInterval(
    _In_ SENSOROBJECT SensorInstance,
    _In_ ULONG DataRateMs
    )
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (pDevice == nullptr || DataRateMs == 0)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Invalid parameter!");
        goto Exit;
    }

    pDevice->m_IntervalMs = DataRateMs;

    // reschedule sample to return as soon as possible if it's started
    if (FALSE != pDevice->m_Started)
    {
        pDevice->m_Started = FALSE;
        WdfTimerStop(pDevice->m_Timer, TRUE);

        pDevice->m_Started = TRUE;
        pDevice->m_FirstSample = TRUE;
        WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(pDevice->m_MinimumIntervalMs));
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



//------------------------------------------------------------------------------
// Function: OnGetDataThresholds
//
// Called by Sensor CLX to get data thresholds. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size
// for the buffer, allocate buffer, then call the function again to retrieve
// sensor information.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//      pThresholds: INOUT_OPT: pointer to a list of sensor thresholds
//      pSize: OUT: number of bytes for the list of sensor thresholds
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnGetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,
    _Inout_opt_ PSENSOR_COLLECTION_LIST pThresholds,
    _Out_ PULONG pSize
    )
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Invalid parameters!");
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
            TraceError("COMBO %!FUNC! Buffer is too small!");
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall (pThresholds, pDevice->m_pThresholds);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
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



//------------------------------------------------------------------------------
// Function: OnSetDataThresholds
//
// Called by Sensor CLX to set data thresholds.
//
// Arguments:
//      SensorInstance: IN: sensor device object
//      pThresholds: IN: pointer to a list of sensor thresholds
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
OnSetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,
    _In_ PSENSOR_COLLECTION_LIST pThresholds
    )
{
    ULONG Element;
    BOOLEAN IsLocked = FALSE;
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (pDevice == nullptr)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! Invalid parameter!");
        goto Exit;
    }

    Lock(pDevice->m_Lock);
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
            TraceError("COMBO %!FUNC! Sensor does NOT have threshold for this data field. Failed %!STATUS!", Status);
            goto Exit;
        }
    }

    // Update cached threshholds
    Status = pDevice->UpdateCachedThreshold();
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! UpdateCachedThreshold failed! %!STATUS!", Status);
        goto Exit;
    }

Exit:
    if (IsLocked)
    {
        Unlock(pDevice->m_Lock);
        IsLocked = FALSE;
    }
    SENSOR_FunctionExit(Status);
    return Status;
}



//------------------------------------------------------------------------------
// Function: OnIoControl
//
// Called by Sensor CLX to handle IOCTLs that clx does not support
//
// Arguments:
//      SensorInstance: IN: Sensor object
//      Request: IN: WDF request object
//      OutputBufferLength: IN: number of bytes to retrieve from output buffer
//      InputBufferLength: IN: number of bytes to retrieve from input buffer
//      IoControlCode: IN: IOCTL control code
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS OnIoControl(
    _In_ SENSOROBJECT /*SensorInstance*/, // Sensor object
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



// Called by Sensor CLX to begin keeping history
NTSTATUS OnStartHistory(_In_ SENSOROBJECT SensorInstance)
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! GetContextFromSensorInstance failed %!STATUS!", Status);
    }
    else
    {
        Status = pDevice->StartHistory();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to stop keeping history.
NTSTATUS OnStopHistory(_In_ SENSOROBJECT SensorInstance)
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! GetContextFromSensorInstance failed %!STATUS!", Status);
    }
    else
    {
        Status = pDevice->StopHistory();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to enable wake
NTSTATUS OnEnableWake(_In_ SENSOROBJECT SensorInstance)
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! GetContextFromSensorInstance failed %!STATUS!", Status);
    }
    else
    {
        pDevice->m_WakeEnabled = TRUE;
        Status = pDevice->EnableWake();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to disable wake
NTSTATUS OnDisableWake(_In_ SENSOROBJECT SensorInstance)
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! GetContextFromSensorInstance failed %!STATUS!", Status);
    }
    else
    {
        pDevice->m_WakeEnabled = FALSE;
        Status = pDevice->DisableWake();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to clear all history stored in the sensor.
NTSTATUS OnClearHistory(_In_ SENSOROBJECT SensorInstance)
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! GetContextFromSensorInstance failed %!STATUS!", Status);
    }
    else
    {
        Status = pDevice->ClearHistory();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to start retrieving history.
NTSTATUS OnStartHistoryRetrieval(
    _In_ SENSOROBJECT SensorInstance,
    _Inout_updates_bytes_(HistorySizeInBytes) PSENSOR_COLLECTION_LIST pHistoryBuffer,
    _In_ ULONG HistorySizeInBytes)
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! GetContextFromSensorInstance failed %!STATUS!", Status);
    }
    else if (sizeof(SENSOR_COLLECTION_LIST) > HistorySizeInBytes)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        TraceError("COMBO %!FUNC! HistorySizeInBytes is too small %!STATUS!", Status);
    }
    else
    {
        Status = pDevice->StartHistoryRetrieval(pHistoryBuffer, HistorySizeInBytes);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to cancel history retrieval.
NTSTATUS OnCancelHistoryRetrieval(_In_ SENSOROBJECT SensorInstance, _Out_ PULONG pBytesWritten)
{
    PComboDevice pDevice = GetContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("COMBO %!FUNC! GetContextFromSensorInstance failed %!STATUS!", Status);
    }
    else
    {
        Status = pDevice->CancelHistoryRetrieval(pBytesWritten);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}