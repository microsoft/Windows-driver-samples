//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of driver callback function
//    from clx to FusionSensor.
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
FusionSensorDevice::GetData(
)
{
    // TODO: Remove the HardwareSimulator code in your final driver. The HardwareSimulator is only used for the purpose of demonstrating how sensor driver samples work.
    PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(m_SimulatorInstance);
    BOOLEAN DataReady = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    FusionSensorSample Sample = {};

    SENSOR_FunctionEnter();

    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("FUS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // TODO: In this case, we are calling into the HardwareSimulator code to get some simulated data. 
    // In a real driver (which either communicates with real hardware or some other drivers), this call should be replaced by some 
    // logic to get data comming from the hardware/other drivers. The "Sample" variable is expected to contain actual data from here on.
    Status = pSimulator->GetSample(&Sample);
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! GetSample failed %!STATUS!", Status);
        goto Exit;
    }

    if (FALSE != m_FirstSample)
    {
        Status = GetPerformanceTime(&m_StartTime);
        if (!NT_SUCCESS(Status))
        {
            m_StartTime = 0;
            TraceError("FUS %!FUNC! GetPerformanceTime failed %!STATUS!", Status);
        }

        m_SampleCount = 0;

        DataReady = TRUE;
    }
    else
    {
        // Compare the change of data to threshold, and only push the data back to 
        // clx if the change exceeds threshold.

        CQUATERNION Quaternion = Sample.Quaternion;
        FLOAT RotationAngle = Quaternion.ToAngleAxis(nullptr) * RadToDegRatio;

        CQUATERNION LastQuaternion = m_LastSample.Quaternion;
        FLOAT LastRotationAngle = LastQuaternion.ToAngleAxis(nullptr) * RadToDegRatio;

        TraceData("FUS %!FUNC! Rotation Angle: new=%f, old=%f", RotationAngle, LastRotationAngle);

        if ((abs(RotationAngle - LastRotationAngle) >= m_CachedThresholds.RotationAngle))
        {
            DataReady = TRUE;
        }
    }

    if (FALSE != DataReady)
    {
        // update last sample
        m_LastSample.Quaternion = Sample.Quaternion;
        m_LastSample.Accuracy = Sample.Accuracy;
        m_LastSample.DeclinationAngle = Sample.DeclinationAngle;

        // push to clx
        InitPropVariantFromFileTime(&m_LastSample.Timestamp, &(m_pData->List[FUSIONSENSOR_DATA_TIMESTAMP].Value));
        InitPropVariantFromFloat(m_LastSample.Quaternion.W, &(m_pData->List[FUSIONSENSOR_DATA_QUATERNION_W].Value));
        InitPropVariantFromFloat(m_LastSample.Quaternion.X, &(m_pData->List[FUSIONSENSOR_DATA_QUATERNION_X].Value));
        InitPropVariantFromFloat(m_LastSample.Quaternion.Y, &(m_pData->List[FUSIONSENSOR_DATA_QUATERNION_Y].Value));
        InitPropVariantFromFloat(m_LastSample.Quaternion.Z, &(m_pData->List[FUSIONSENSOR_DATA_QUATERNION_Z].Value));
        InitPropVariantFromUInt32(m_LastSample.Accuracy, &(m_pData->List[FUSIONSENSOR_DATA_ACCURACY].Value));
        InitPropVariantFromFloat(m_LastSample.DeclinationAngle, &(m_pData->List[FUSIONSENSOR_DATA_DECLINATION_ANGLE].Value));


        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("FUS %!FUNC! Data did NOT meet the threshold");
    }

    SENSOR_FunctionExit(Status);

Exit:
    return Status;
}



// This callback is called when interval wait time has expired and driver is ready
// to collect new sample. The callback reads current value, compare value to threshold,
// pushes it up to CLX framework, and schedule next wake up time.
VOID
FusionSensorDevice::OnTimerExpire(
    _In_ WDFTIMER Timer // WDF timer object
    )
{
    PFusionSensorDevice pDevice = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    pDevice = GetFusionSensorContextFromSensorInstance(WdfTimerGetParentObject(Timer));
    if (nullptr == pDevice)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("FUS %!FUNC! GetFusionSensorContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Get data and push to clx
    WdfWaitLockAcquire(pDevice->m_Lock, NULL);
    Status = pDevice->GetData();
    if (!NT_SUCCESS(Status) && Status != STATUS_DATA_NOT_ACCEPTED)
    {
        TraceError("FUS %!FUNC! GetData Failed %!STATUS!", Status);
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
                TraceError("FUS %!FUNC! GetPerformanceTime %!STATUS!", Status);
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
FusionSensorDevice::OnStart(
    _In_ SENSOROBJECT SensorInstance // Sensor device object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    // Get the simulator context
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("FUS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
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
        WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(FusionSensor_Default_MinDataInterval_Ms));
    }
Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to stop continuously sampling the sensor.
NTSTATUS
FusionSensorDevice::OnStop(
    _In_ SENSOROBJECT SensorInstance // Sensor device object
    )
{
    PHardwareSimulator pSimulator = nullptr;
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();
    
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
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
        TraceError("FUS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator->Stop();

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get supported data fields. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS
FusionSensorDevice::OnGetSupportedDataFields(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _Inout_opt_ PSENSOR_PROPERTY_LIST pFields, // Pointer to a list of supported properties
    _Out_ PULONG pSize // Number of bytes for the list of supported properties
    )
{
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Invalid parameters! %!STATUS!", Status);
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
            TraceError("FUS %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out data
        Status = PropertiesListCopy(pFields, pDevice->m_pSupportedDataFields);
        if (!NT_SUCCESS(Status))
        {
            TraceError("FUS %!FUNC! PropertiesListCopy failed %!STATUS!", Status);
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
FusionSensorDevice::OnGetProperties(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // Pointer to a list of sensor properties
    _Out_ PULONG pSize // Number of bytes for the list of sensor properties
    )
{
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Invalid parameters! %!STATUS!", Status);
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
            TraceError("FUS %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pProperties);
        if (!NT_SUCCESS(Status))
        {
            TraceError("FUS %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
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
FusionSensorDevice::OnGetDataFieldProperties(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _In_ const PROPERTYKEY *DataField, // Pointer to the propertykey of requested property
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // Pointer to a list of sensor properties
    _Out_ PULONG pSize // Number of bytes for the list of sensor properties
    )
{
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize || nullptr == DataField)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Invalid parameters! %!STATUS!", Status);
        goto Exit;
    }
    

    if ((*DataField == PKEY_SensorData_LinearAccelerationX_Gs) ||
        (*DataField == PKEY_SensorData_LinearAccelerationY_Gs) ||
        (*DataField == PKEY_SensorData_LinearAccelerationZ_Gs))
    {
        // Linear Acceleration
        if (nullptr == pProperties)
        {
            // Just return size
            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pAccDataFieldProperties);
        }
        else
        {
            if (pProperties->AllocatedSizeInBytes <
                CollectionsListGetMarshalledSize(pDevice->m_pAccDataFieldProperties))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("FUS %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
                goto Exit;
            }

            // Fill out all data
            Status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pAccDataFieldProperties);
            if (!NT_SUCCESS(Status))
            {
                TraceError("FUS %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
                goto Exit;
            }

            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pAccDataFieldProperties);
        }
    }
    else if ((*DataField == PKEY_SensorData_CorrectedAngularVelocityX_DegreesPerSecond) ||
        (*DataField == PKEY_SensorData_CorrectedAngularVelocityY_DegreesPerSecond) ||
        (*DataField == PKEY_SensorData_CorrectedAngularVelocityZ_DegreesPerSecond))
    {
        // Rotation Rate
        if (nullptr == pProperties)
        {
            // Just return size
            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pGyrDataFieldProperties);
        }
        else
        {
            if (pProperties->AllocatedSizeInBytes <
                CollectionsListGetMarshalledSize(pDevice->m_pGyrDataFieldProperties))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("FUS %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
                goto Exit;
            }

            // Fill out all data
            Status = CollectionsListCopyAndMarshall(pProperties, pDevice->m_pGyrDataFieldProperties);
            if (!NT_SUCCESS(Status))
            {
                TraceError("FUS %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
                goto Exit;
            }

            *pSize = CollectionsListGetMarshalledSize(pDevice->m_pGyrDataFieldProperties);
        }
    }
    else
    {
        Status = STATUS_NOT_SUPPORTED;
        TraceError("FUS %!FUNC! Fusion sensor does NOT have properties for this data field. Failed %!STATUS!", Status);
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
FusionSensorDevice::OnGetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _Out_ PULONG DataRateMs // Sampling rate in ms
    )
{
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    if (nullptr == DataRateMs)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! DataRateMs(0x%p) parameter is invalid. Failed %!STATUS!", DataRateMs, Status);
        goto Exit;
    }

    *DataRateMs = pDevice->m_Interval;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// Called by Sensor CLX to set sampling rate of the sensor.
NTSTATUS
FusionSensorDevice::OnSetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _In_ ULONG DataRateMs // Sampling rate in ms
    )
{
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
        goto Exit;
    }

    if (FusionSensor_Default_MinDataInterval_Ms > DataRateMs)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! DataRateMs(%d) parameter is smaller than the minimum data interval. Failed %!STATUS!", DataRateMs, Status);
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
        WdfTimerStart(pDevice->m_Timer, WDF_REL_TIMEOUT_IN_MS(FusionSensor_Default_MinDataInterval_Ms));
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
FusionSensorDevice::OnGetDataThresholds(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pThresholds, // Pointer to a list of sensor thresholds
    _Out_ PULONG pSize // Number of bytes for the list of sensor thresholds
    )
{
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pDevice || nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Invalid parameters! %!STATUS!", Status);
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
            TraceError("FUS %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            goto Exit;
        }

        // Fill out all data
        Status = CollectionsListCopyAndMarshall(pThresholds, pDevice->m_pThresholds);
        if (!NT_SUCCESS(Status))
        {
            TraceError("FUS %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
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
FusionSensorDevice::OnSetDataThresholds(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _In_ PSENSOR_COLLECTION_LIST pThresholds // Pointer to a list of sensor thresholds
    )
{
    ULONG Element;
    BOOLEAN IsLocked = FALSE;
    PFusionSensorDevice pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (pDevice == nullptr)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! Sensor(0x%p) parameter is invalid. Failed %!STATUS!", SensorInstance, Status);
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
            TraceError("FUS %!FUNC! FusionSensor driver does NOT have threshold for this data field. Failed %!STATUS!", Status);
            goto Exit;
        }
    }

    // Get data thresholds
    Status = PropKeyFindKeyGetFloat(pDevice->m_pThresholds,
        &PKEY_SensorData_RotationAngle_Degrees,
        &(pDevice->m_CachedThresholds.RotationAngle));
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! PropKeyFindKeyGetFloat for rotation angle failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(pDevice->m_pThresholds,
        &PKEY_SensorData_LinearAccelerationX_Gs,
        &(pDevice->m_CachedThresholds.LinearAcceleration.X));
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! PropKeyFindKeyGetFloat for linear acceleration X failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(pDevice->m_pThresholds,
        &PKEY_SensorData_LinearAccelerationY_Gs,
        &(pDevice->m_CachedThresholds.LinearAcceleration.Y));
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! PropKeyFindKeyGetFloat for linear acceleration Y failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(pDevice->m_pThresholds,
        &PKEY_SensorData_LinearAccelerationZ_Gs,
        &(pDevice->m_CachedThresholds.LinearAcceleration.Z));
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! PropKeyFindKeyGetFloat for linear acceleration Z failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(pDevice->m_pThresholds,
        &PKEY_SensorData_CorrectedAngularVelocityX_DegreesPerSecond,
        &(pDevice->m_CachedThresholds.RotationRate.X));
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! PropKeyFindKeyGetFloat for rotation rate X failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(pDevice->m_pThresholds,
        &PKEY_SensorData_CorrectedAngularVelocityY_DegreesPerSecond,
        &(pDevice->m_CachedThresholds.RotationRate.Y));
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! PropKeyFindKeyGetFloat for rotation rate Y failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(pDevice->m_pThresholds,
        &PKEY_SensorData_CorrectedAngularVelocityZ_DegreesPerSecond,
        &(pDevice->m_CachedThresholds.RotationRate.Z));
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! PropKeyFindKeyGetFloat for rotation rate Z failed! %!STATUS!", Status);
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
FusionSensorDevice::OnIoControl(
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