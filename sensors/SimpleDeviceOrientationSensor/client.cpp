//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of driver callback function
//    from clx to simple device orientation sensor.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include <timeapi.h>

#include "Client.tmh"

// This routine is called by worker thread to read a single sample, compare threshold
// and push it back to CLX.
// Returns an NTSTATUS code
NTSTATUS SdoDevice::GetData()
{
    PHardwareSimulator pSimulator = nullptr;
    FILETIME TimeStamp = {};
    NTSTATUS Status = STATUS_SUCCESS;
    ABI::Windows::Devices::Sensors::SimpleOrientation Sample;

    SENSOR_FunctionEnter();

    if (FALSE != m_FirstSample)
    {
        Status = GetPerformanceTime(&m_StartTime);
        if (!NT_SUCCESS(Status))
        {
            m_StartTime = 0;
            TraceError("SDOS %!FUNC! GetPerformanceTime %!STATUS!", Status);
        }
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("SDOS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    Sample = pSimulator->GetOrientation();

    if (FALSE != m_FirstSample || m_LastSample != Sample)
    {
        m_LastSample = Sample;

        // push to clx
        InitPropVariantFromUInt32(m_LastSample, &(m_pData->List[SDO_DATA_SIMPLEDEVICEORIENTATION].Value));

        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[SDO_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);

        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("SDOS %!FUNC! SDO Data did NOT meet the threshold");
    }

Exit:
    SENSOR_FunctionExit(Status);

    return Status;
}

// This callback is called when interval wait time has expired and driver is ready
// to collect new sample. The callback reads current value, compare value to threshold,
// pushes it up to CLX framework, and schedule next wake up time.
VOID SdoDevice::OnTimerExpire(
    _In_ WDFTIMER Timer // WDF timer object
    )
{
    PSdoDevice pDevice = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    pDevice = GetSdoContextFromSensorInstance(WdfTimerGetParentObject(Timer));
    if (nullptr == pDevice)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("SDOS %!FUNC! GetSdoContextFromSensorInstance failed %!STATUS!", Status);
    }

    if (NT_SUCCESS(Status))
    {
        // Get data and push to clx
        WdfWaitLockAcquire(pDevice->m_Lock, NULL);
        Status = pDevice->GetData();
        if (!NT_SUCCESS(Status) && Status != STATUS_DATA_NOT_ACCEPTED)
        {
            TraceError("SDOS %!FUNC! GetData Failed %!STATUS!", Status);
        }
        WdfWaitLockRelease(pDevice->m_Lock);

        // Schedule next wake up time
        if (Sdo_Mininum_DataInterval <= pDevice->m_Interval &&
            FALSE != pDevice->m_PoweredOn &&
            FALSE != pDevice->m_Started)
        {
            LONGLONG WaitTime = 0;  // in unit of 100ns

            if (0 == pDevice->m_StartTime)
            {
                // in case we fail to get sensor start time, use static wait time
                WaitTime = WDF_REL_TIMEOUT_IN_MS(pDevice->m_Interval);
            }
            else
            {
                ULONG CurrentTimeMs = 0;

                // dynamically calculate wait time to avoid jitter
                Status = GetPerformanceTime(&CurrentTimeMs);
                if (!NT_SUCCESS(Status))
                {
                    TraceError("SDOS %!FUNC! GetPerformanceTime %!STATUS!", Status);
                    WaitTime = WDF_REL_TIMEOUT_IN_MS(pDevice->m_Interval);
                }
                else
                {
                    WaitTime = pDevice->m_Interval -
                        ((CurrentTimeMs - pDevice->m_StartTime) % pDevice->m_Interval);
                    WaitTime = WDF_REL_TIMEOUT_IN_MS(WaitTime);
                }
            }
            WdfTimerStart(pDevice->m_Timer, WaitTime);
        }
    }

    SENSOR_FunctionExit(Status);
}

// Called by Sensor CLX to begin continously sampling the sensor.
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnStart(
    _In_ SENSOROBJECT SensorInstance // sensor device object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    PSdoDevice pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT)SensorInstance, Status);
    }
    else if (0 == pDevice->m_Interval)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! Interval parameter is invalid (equal to 0). Failed %!STATUS!", Status);
    }

    if (NT_SUCCESS(Status))
    {
        // Get the simulator context
        pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
        if (nullptr == pSimulator)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("SDOS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
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
        // Note: The polling timer is configured to allow for the first sample to be reported immediately.
        // Some hardware may want to delay the first sample report a little to account for hardware start time.
        WdfTimerStart(pDevice->m_Timer, 0);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to stop continously sampling the sensor.
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnStop(
    _In_ SENSOROBJECT SensorInstance // sensor device object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    PSdoDevice pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();
    
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT)SensorInstance, Status);
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
            TraceError("SDOS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
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
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnGetSupportedDataFields(
    _In_ SENSOROBJECT SensorInstance,          // sensor device object
    _Inout_opt_ PSENSOR_PROPERTY_LIST pFields, // pointer to a list of supported properties
    _Out_ PULONG pSize                         // number of bytes for the list of supported properties
    )
{
    PSdoDevice pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG size = 0;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
        goto Exit;
    }

    *pSize = 0;

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! pDevice: Invalid parameter! %!STATUS!", Status);
        goto Exit;
    }

    if (nullptr == pFields)
    {
        // Just return size
        size = pDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
    }
    else 
    {
        if (pFields->AllocatedSizeInBytes < pDevice->m_pSupportedDataFields->AllocatedSizeInBytes)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("SDOS %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out data
        Status = PropertiesListCopy(pFields, pDevice->m_pSupportedDataFields);
        if (!NT_SUCCESS(Status))
        {
            TraceError("SDOS %!FUNC! PropertiesListCopy failed %!STATUS!", Status);
            goto Exit;
        }

        size = pDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
    }

    *pSize = size;

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
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnGetProperties(
    _In_ SENSOROBJECT SensorInstance,                // sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // pointer to a list of sensor properties
    _Out_ PULONG pSize                               // number of bytes for the list of sensor properties
    )
{
    PSdoDevice pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG size = 0;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
        goto Exit;
    }

    *pSize = 0;

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! pDevice: Invalid parameter! %!STATUS!", Status);
        goto Exit;
    }

    if (nullptr == pProperties)
    {
        // Just return size
        size = CollectionsListGetMarshalledSize(pDevice->m_pProperties);
    }
    else 
    {
        if (pProperties->AllocatedSizeInBytes < 
            CollectionsListGetMarshalledSize(pDevice->m_pProperties))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("SDOS %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pProperties);
        if (!NT_SUCCESS(Status))
        {
            TraceError("SDOS %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
            goto Exit;
        }

        size = CollectionsListGetMarshalledSize(pDevice->m_pProperties);
    }

    *pSize = size;

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
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnGetDataFieldProperties(
    _In_ SENSOROBJECT SensorInstance,                       // sensor device object
    _In_ const PROPERTYKEY *DataField,                      // pointer to the propertykey of requested property
    _Inout_opt_ PSENSOR_COLLECTION_LIST /*pProperties*/,    // pointer to a list of sensor properties
    _Out_ PULONG pSize                                      // number of bytes for the list of sensor properties
)
{
    PSdoDevice pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize || nullptr == DataField)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! Invalid parameters! %!STATUS!", Status);
    }

    *pSize = 0;

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get sampling rate of the sensor.
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnGetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // sensor device object
    _Out_ PULONG DataRateMs           // sampling rate in milliseconds
    )
{
    PSdoDevice pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT)SensorInstance, Status);
        goto Exit;
    }

    if (nullptr == DataRateMs)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! DataRateMs(%08X) parameter is invalid. Failed %!STATUS!", (INT)DataRateMs, Status);
        goto Exit;
    }

    *DataRateMs = pDevice->m_Interval;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to set sampling rate of the sensor.
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnSetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // sensor device object
    _In_ ULONG DataRateMs             // sampling rate in milliseconds
    )
{
    PSdoDevice pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || Sdo_Mininum_DataInterval > DataRateMs)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! Sensor(%08X) parameter is invalid. Failed %!STATUS!", (INT)SensorInstance, Status);
        goto Exit;
    }

    pDevice->m_Interval = DataRateMs;

    // Restart the timer at minimum report interval to return a sample as soon as possible.
    if (FALSE != pDevice->m_Started)
    {
        pDevice->m_Started = FALSE;
        WdfTimerStop(pDevice->m_Timer, TRUE);

        pDevice->m_Started = TRUE;
        pDevice->m_FirstSample = TRUE;
        WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(Sdo_Mininum_DataInterval));
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get data thresholds. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnGetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,                // sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pThresholds, // pointer to a list of sensor thresholds
    _Out_ PULONG pSize                               // number of bytes for the list of sensor thresholds
    )
{
    PSdoDevice pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }

    if (nullptr == pThresholds)
    {
        // Just return size
        *pSize = CollectionsListGetMarshalledSize(pDevice->m_pEmptyThreshold);
    }
    else
    {
        if (pThresholds->AllocatedSizeInBytes <
            CollectionsListGetMarshalledSize(pDevice->m_pEmptyThreshold))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("SDOS %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall(pThresholds, pDevice->m_pEmptyThreshold);
        if (!NT_SUCCESS(Status))
        {
            TraceError("SDOS %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
            goto Exit;
        }

        *pSize = CollectionsListGetMarshalledSize(pDevice->m_pEmptyThreshold);
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
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnSetDataThresholds(
    _In_ SENSOROBJECT /*SensorInstance*/,         // sensor device object
    _In_ PSENSOR_COLLECTION_LIST /*pThresholds*/  // pointer to a list of sensor thresholds
    )
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    SENSOR_FunctionEnter();

    // Unsupported at this point in time.

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to handle IOCTLs that clx does not support
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnIoControl(
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
