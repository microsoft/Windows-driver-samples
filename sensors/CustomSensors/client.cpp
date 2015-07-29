//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of driver callback function
//    from clx to custom sensors.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include <timeapi.h>

#include "Client.tmh"

// This routine is called by worker thread to read a single sample and push it back 
// to CLX.
NTSTATUS CustomSensorDevice::GetData()
{
    PHardwareSimulator pSimulator = nullptr;
    FILETIME TimeStamp = {};
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (FALSE != m_FirstSample)
    {
        Status = GetPerformanceTime(&m_StartTime);
        if (!NT_SUCCESS(Status))
        {
            m_StartTime = 0;
            TraceError("CSTM %!FUNC! GetPerformanceTime %!STATUS!", Status);
        }
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("CSTM %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // push to clx
    InitPropVariantFromFloat(pSimulator->GetSample(), &(m_pData->List[CSTM_DATA_CO2_LEVEL_PERCENT].Value));

    GetSystemTimePreciseAsFileTime(&TimeStamp);
    InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[CSTM_DATA_TIMESTAMP].Value));

    SensorsCxSensorDataReady(m_SensorInstance, m_pData);
    m_FirstSample = FALSE;

Exit:
    SENSOR_FunctionExit(Status);

    return Status;
}



// This callback is called when interval wait time has expired and driver is ready
// to collect new sample. The callback reads current value,
// pushes it up to CLX framework, and schedule next wake up time.
VOID
CustomSensorDevice::OnTimerExpire(
    _In_ WDFTIMER Timer // WDF timer object
    )
{
    PCustomSensorDevice pDevice = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    pDevice = GetCustomSensorContextFromSensorInstance(WdfTimerGetParentObject(Timer));
    if (nullptr == pDevice)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("CSTM %!FUNC! GetCustomSensorContextFromSensorInstance failed %!STATUS!", Status);
    }

    if (NT_SUCCESS(Status))
    {
        // Get data and push to clx
        WdfWaitLockAcquire(pDevice->m_Lock, NULL);
        Status = pDevice->GetData();
        if (!NT_SUCCESS(Status) && Status != STATUS_DATA_NOT_ACCEPTED)
        {
            TraceError("CSTM %!FUNC! GetCstmData Failed %!STATUS!", Status);
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
                Status = GetPerformanceTime(&CurrentTimeMs);
                if (!NT_SUCCESS(Status))
                {
                    TraceError("PED %!FUNC! GetPerformanceTime %!STATUS!", Status);
                    WaitTimeHundredNanoseconds = WDF_REL_TIMEOUT_IN_MS(pDevice->m_Interval);
                }
                else
                {
                    WaitTimeHundredNanoseconds = pDevice->m_Interval -
                        ((CurrentTimeMs - pDevice->m_StartTime) % pDevice->m_Interval);
                    WaitTimeHundredNanoseconds = WDF_REL_TIMEOUT_IN_MS(WaitTimeHundredNanoseconds);
                }
            }
            WdfTimerStart(pDevice->m_Timer, WaitTimeHundredNanoseconds);
        }
    }

    SENSOR_FunctionExit(Status);
}



// Called by Sensor CLX to begin continously sampling the sensor.
NTSTATUS
CustomSensorDevice::OnStart(
    _In_ SENSOROBJECT SensorInstance // sensor device object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    PCustomSensorDevice pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT)SensorInstance, Status);
    }

    if (NT_SUCCESS(Status))
    {
        // Get the simulator context
        pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
        if (nullptr == pSimulator)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        }
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
        // Note1: the WDF timer is only as precise as the system resolution allows it to be.
        // In the case of the CO2 sensor, the reporting interval is 200 milliseconds. The default 
        // system resolution (15.6 milliseconds) is therefore fine enough to guarantee an accurate sample 
        // reporting interval. Some sensors using a lower reporting interval may want to reduce the system 
        // time resolution by calling into timeBeginPeriod() before starting the polling timer.
        //
        // Important consideration: calling into timeBeginPeriod() should be used with care as it has 
        // an adverse on the system performance and power consumption.
        //
        // Note2: The polling timer is configured to allow for the first sample to be reported immediately.
        // Some hardware may want to delay the first sample report a little to account for hardware start time.
        WdfTimerStart(pDevice->m_Timer, 0);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to stop continously sampling the sensor.
NTSTATUS
CustomSensorDevice::OnStop(
    _In_ SENSOROBJECT SensorInstance // sensor device object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    PCustomSensorDevice pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();
    
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT) SensorInstance, Status);
    }

    if (NT_SUCCESS(Status))
    {
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
            TraceError("CSTM %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
            goto Exit;
        }

        pSimulator->Stop();
    }

Exit:
    SENSOR_FunctionExit(Status);

    return Status;
}




// Called by Sensor CLX to get supported data fields. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS
CustomSensorDevice::OnGetSupportedDataFields(
    _In_ SENSOROBJECT SensorInstance,          // sensor device object
    _Inout_opt_ PSENSOR_PROPERTY_LIST pFields, // pointer to a list of supported properties
    _Out_ PULONG pSize                         // number of bytes for the list of supported properties
    )
{
    PCustomSensorDevice pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
        goto Exit;
    }

    *pSize = 0;

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! pDevice: Invalid parameter! %!STATUS!", Status);
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
            TraceError("CSTM %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out data
        Status = PropertiesListCopy (pFields, pDevice->m_pSupportedDataFields);
        if (!NT_SUCCESS(Status))
        {
            TraceError("CSTM %!FUNC! PropertiesListCopy failed %!STATUS!", Status);
            goto Exit;
        }

        *pSize = pDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to get sensor properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS
CustomSensorDevice::OnGetProperties(
    _In_ SENSOROBJECT SensorInstance,                // sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // pointer to a list of sensor properties
    _Out_ PULONG pSize                               // number of bytes for the list of sensor properties
    )
{
    PCustomSensorDevice pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
        goto Exit;
    }

    *pSize = 0;

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! pDevice: Invalid parameter! %!STATUS!", Status);
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
            TraceError("CSTM %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pProperties);
        if (!NT_SUCCESS(Status))
        {
            TraceError("CSTM %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
            goto Exit;
        }

        *pSize = CollectionsListGetMarshalledSize(pDevice->m_pProperties);
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}


// Called by Sensor CLX to get data field properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS
CustomSensorDevice::OnGetDataFieldProperties(
    _In_ SENSOROBJECT SensorInstance,                // sensor device object
    _In_ const PROPERTYKEY *DataField,               // pointer to the propertykey of requested property
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // pointer to a list of sensor properties
    _Out_ PULONG pSize                               // number of bytes for the list of sensor properties
)
{
    PCustomSensorDevice pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
        goto Exit;
    }

    *pSize = 0;

    if (nullptr == pDevice || nullptr == DataField)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }

    if ((*DataField == pDevice->m_pSupportedDataFields->List[CSTM_DATA_CO2_LEVEL_PERCENT]))
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
                TraceError("CSTM %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
                goto Exit;
            }

            // Fill out all data
            Status = CollectionsListCopyAndMarshall (pProperties, pDevice->m_pDataFieldProperties);
            if (!NT_SUCCESS(Status))
            {
                TraceError("CSTM %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
                goto Exit;
            }

            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pDataFieldProperties);
        }
    }
    else
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("CSTM %!FUNC! CustomSensor does NOT have properties for this data field. Failed %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to get sampling rate of the sensor.
NTSTATUS
CustomSensorDevice::OnGetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // sensor device object
    _Out_ PULONG DataRateMs           // sampling rate in milliseconds
    )
{
    PCustomSensorDevice pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT) SensorInstance, Status);
        goto Exit;
    }

    if (nullptr == DataRateMs)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! DataRateMs(%08X) parameter is invalid. Failed %!STATUS!", (INT)DataRateMs, Status);
        goto Exit;
    }

    *DataRateMs = pDevice->m_Interval;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to set sampling rate of the sensor.
NTSTATUS
CustomSensorDevice::OnSetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // sensor device object
    _In_ ULONG DataRateMs             // sampling rate in milliseconds
    )
{
    PCustomSensorDevice pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || Cstm_Default_MinDataInterval_Ms > DataRateMs)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT) SensorInstance, Status);
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
        WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(Cstm_Default_MinDataInterval_Ms));
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// UNSUPPORTED at this point of time
NTSTATUS
CustomSensorDevice::OnGetDataThresholds(
    _In_ SENSOROBJECT /*SensorInstance*/,                // sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST /*pThresholds*/, // pointer to a list of sensor thresholds
    _Out_ PULONG pSize                                   // number of bytes for the list of sensor thresholds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SENSOR_COLLECTION_LIST emptyList = {};

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }

    // Even though thresholds are not yet supported for custom sensors,
    // the minimum reported size must be that of an empty list for the 
    // class extension to work properly
    emptyList.AllocatedSizeInBytes = sizeof(emptyList);

    *pSize = CollectionsListGetMarshalledSize(&emptyList);

    SENSOR_FunctionExit(Status);

Exit:
    return Status;
}



// UNSUPPORTED at this point of time
NTSTATUS
CustomSensorDevice::OnSetDataThresholds(
    _In_ SENSOROBJECT /*SensorInstance*/,         // sensor device object
    _In_ PSENSOR_COLLECTION_LIST /*pThresholds*/  // pointer to a list of sensor thresholds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Usupported at this point in time

    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to handle IOCTLs that clx does not support
NTSTATUS
CustomSensorDevice::OnIoControl(
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
