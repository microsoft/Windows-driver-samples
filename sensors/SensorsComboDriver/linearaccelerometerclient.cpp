// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//
//  This module contains the implementation of sensor specific functions.
//
// Environment:
//
//  Windows User-Mode Driver Framework (UMDF)

#include "Clients.h"

#include "LinearAccelerometerClient.tmh"

#define SENSORV2_POOL_TAG_LINEAR_ACCELEROMETER               '2CaL'

#define LinearAccelerometerDevice_Default_MinDataInterval (4)
#define LinearAccelerometerDevice_Default_Axis_Threshold  (1.0f)
#define LinearAccelerometerDevice_Axis_Resolution         (4.0f / 65536.0f) // in delta g
#define LinearAccelerometerDevice_Axis_Minimum            (-2.0f)           // in g
#define LinearAccelerometerDevice_Axis_Maximum            (2.0f)            // in g

// Linear Accelerometer Unique ID
// {2BAAA1A7-6795-42A0-B830-82526CFD28D1}
DEFINE_GUID(GUID_LinearAccelerometerDevice_UniqueID,
    0x2baaa1a7, 0x6795, 0x42a0, 0xb8, 0x30, 0x82, 0x52, 0x6c, 0xfd, 0x28, 0xd1);

// Sensor data
typedef enum
{
    LINEAR_ACCELEROMETER_DATA_X = 0,
    LINEAR_ACCELEROMETER_DATA_Y,
    LINEAR_ACCELEROMETER_DATA_Z,
    LINEAR_ACCELEROMETER_DATA_TIMESTAMP,
    LINEAR_ACCELEROMETER_DATA_SHAKE,
    LINEAR_ACCELEROMETER_DATA_COUNT
} LINEAR_ACCELEROMETER_DATA_INDEX;

//------------------------------------------------------------------------------
// Function: Initialize
//
// This routine initializes the sensor to its default properties
//
// Arguments:
//       Device: IN: WDFDEVICE object
//       SensorInstance: IN: SENSOROBJECT for each sensor instance
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
LinearAccelerometerDevice::Initialize(
    _In_ WDFDEVICE Device,
    _In_ SENSOROBJECT SensorInstance
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    //
    // Store device and instance
    //
    m_Device = Device;
    m_SensorInstance = SensorInstance;
    m_Started = FALSE;

    //
    // Create Lock
    //
    Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! LAC WdfWaitLockCreate failed %!STATUS!", Status);
        goto Exit;
    }

    //
    // Create timer object for polling sensor samples
    //
    {
        WDF_OBJECT_ATTRIBUTES TimerAttributes;
        WDF_TIMER_CONFIG TimerConfig;

        WDF_TIMER_CONFIG_INIT(&TimerConfig, OnTimerExpire);
        WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
        TimerAttributes.ParentObject = SensorInstance;
        TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;
        TimerConfig.TolerableDelay = 0;

        Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_Timer);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! LAC WdfTimerCreate failed %!STATUS!", Status);
            goto Exit;
        }
    }

    //
    // Sensor Enumeration Properties
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ENUMERATION_PROPERTIES_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_LINEAR_ACCELEROMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pEnumerationProperties);
        if (!NT_SUCCESS(Status) || m_pEnumerationProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! LAC WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
        m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_LinearAccelerometer,
                                 &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"Manufacturer name",
                                  &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"Linear Accelerometer",
                                  &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Key = DEVPKEY_Sensor_ConnectionType;
        // The DEVPKEY_Sensor_ConnectionType values match the SensorConnectionType enumeration
        InitPropVariantFromUInt32(static_cast<ULONG>(SensorConnectionType::Integrated),
                                 &(m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_LinearAccelerometerDevice_UniqueID,
                                 &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));

        m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Key = DEVPKEY_Sensor_IsPrimary;
        InitPropVariantFromBoolean(FALSE,
                                 &(m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Value));
    }

    //
    // Supported Data-Fields
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_PROPERTY_LIST_SIZE(LINEAR_ACCELEROMETER_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_LINEAR_ACCELEROMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pSupportedDataFields);
        if (!NT_SUCCESS(Status) || m_pSupportedDataFields == nullptr)
        {
            TraceError("COMBO %!FUNC! LAC WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
        m_pSupportedDataFields->Count = LINEAR_ACCELEROMETER_DATA_COUNT;

        m_pSupportedDataFields->List[LINEAR_ACCELEROMETER_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[LINEAR_ACCELEROMETER_DATA_X] = PKEY_SensorData_AccelerationX_Gs;
        m_pSupportedDataFields->List[LINEAR_ACCELEROMETER_DATA_Y] = PKEY_SensorData_AccelerationY_Gs;
        m_pSupportedDataFields->List[LINEAR_ACCELEROMETER_DATA_Z] = PKEY_SensorData_AccelerationZ_Gs;
        m_pSupportedDataFields->List[LINEAR_ACCELEROMETER_DATA_SHAKE] = PKEY_SensorData_Shake;
    }

    //
    // Data
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(LINEAR_ACCELEROMETER_DATA_COUNT);
        FILETIME Time = {0};

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_LINEAR_ACCELEROMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pData);
        if (!NT_SUCCESS(Status) || m_pData == nullptr)
        {
            TraceError("COMBO %!FUNC! LAC WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

    SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
    m_pData->Count = LINEAR_ACCELEROMETER_DATA_COUNT;

    m_pData->List[LINEAR_ACCELEROMETER_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
    GetSystemTimePreciseAsFileTime(&Time);
    InitPropVariantFromFileTime(&Time, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_TIMESTAMP].Value));

    m_pData->List[LINEAR_ACCELEROMETER_DATA_X].Key = PKEY_SensorData_AccelerationX_Gs;
    InitPropVariantFromFloat(0.0, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_X].Value));

    m_pData->List[LINEAR_ACCELEROMETER_DATA_Y].Key = PKEY_SensorData_AccelerationY_Gs;
    InitPropVariantFromFloat(0.0, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_Y].Value));

    m_pData->List[LINEAR_ACCELEROMETER_DATA_Z].Key = PKEY_SensorData_AccelerationZ_Gs;
    InitPropVariantFromFloat(0.0, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_Z].Value));

    m_pData->List[LINEAR_ACCELEROMETER_DATA_SHAKE].Key = PKEY_SensorData_Shake;
    InitPropVariantFromBoolean(FALSE, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_SHAKE].Value));

    m_CachedData.Axis.X = 0.0f;
    m_CachedData.Axis.Y = 0.0f;
    m_CachedData.Axis.Z = -1.0f;
    m_CachedData.Shake = FALSE;

    m_LastSample.Axis.X  = 0.0f;
    m_LastSample.Axis.Y  = 0.0f;
    m_LastSample.Axis.Z  = 0.0f;
    m_LastSample.Shake = FALSE;
    }

    //
    // Sensor Properties
    //
    {
        m_IntervalMs = LinearAccelerometerDevice_Default_MinDataInterval;

        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_COMMON_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_LINEAR_ACCELEROMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pProperties);
        if (!NT_SUCCESS(Status) || m_pProperties == nullptr)
        {
            TraceError("LAC %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
        m_pProperties->Count = SENSOR_COMMON_PROPERTY_COUNT;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(LinearAccelerometerDevice_Default_MinDataInterval,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_LinearAccelerometer,
                                     &(m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Value));
    }

    //
    // Data field properties
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_DATA_FIELD_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_LINEAR_ACCELEROMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pDataFieldProperties);
        if (!NT_SUCCESS(Status) || m_pDataFieldProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! LAC WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
        m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

        m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat(LinearAccelerometerDevice_Axis_Resolution,
                                 &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

        m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(LinearAccelerometerDevice_Axis_Minimum,
                                 &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

        m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(LinearAccelerometerDevice_Axis_Maximum,
                                 &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));
    }

    //
    // Set default threshold
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;

        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(LINEAR_ACCELEROMETER_DATA_COUNT - 2);    //  Timestamp and shake do not have thresholds

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_LINEAR_ACCELEROMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pThresholds);
        if (!NT_SUCCESS(Status) || m_pThresholds == nullptr)
        {
            TraceError("COMBO %!FUNC! LAC WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
        m_pThresholds->Count = LINEAR_ACCELEROMETER_DATA_COUNT - 2;

        m_pThresholds->List[LINEAR_ACCELEROMETER_DATA_X].Key = PKEY_SensorData_AccelerationX_Gs;
        InitPropVariantFromFloat(LinearAccelerometerDevice_Default_Axis_Threshold,
                                 &(m_pThresholds->List[LINEAR_ACCELEROMETER_DATA_X].Value));

        m_pThresholds->List[LINEAR_ACCELEROMETER_DATA_Y].Key = PKEY_SensorData_AccelerationY_Gs;
        InitPropVariantFromFloat(LinearAccelerometerDevice_Default_Axis_Threshold,
                                 &(m_pThresholds->List[LINEAR_ACCELEROMETER_DATA_Y].Value));

        m_pThresholds->List[LINEAR_ACCELEROMETER_DATA_Z].Key = PKEY_SensorData_AccelerationZ_Gs;
        InitPropVariantFromFloat(LinearAccelerometerDevice_Default_Axis_Threshold,
                                 &(m_pThresholds->List[LINEAR_ACCELEROMETER_DATA_Z].Value));

        m_CachedThresholds.Axis.X = LinearAccelerometerDevice_Default_Axis_Threshold;
        m_CachedThresholds.Axis.Y = LinearAccelerometerDevice_Default_Axis_Threshold;
        m_CachedThresholds.Axis.Z = LinearAccelerometerDevice_Default_Axis_Threshold;

        m_FirstSample = TRUE;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



//------------------------------------------------------------------------------
// Function: GetData
//
// This routine is called by worker thread to read a single sample, compare threshold
// and push it back to CLX. It simulates hardware thresholding by only generating data
// when the change of data is greater than threshold.
//
// Arguments:
//       None
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
LinearAccelerometerDevice::GetData(
    )
{
    BOOLEAN DataReady = FALSE;
    FILETIME TimeStamp = {0};
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // new sample?
    if (m_FirstSample != FALSE)
    {
        Status = GetPerformanceTime (&m_StartTime);
        if (!NT_SUCCESS(Status))
        {
            m_StartTime = 0;
            TraceError("COMBO %!FUNC! LAC GetPerformanceTime %!STATUS!", Status);
        }

        m_SampleCount = 0;

        DataReady = TRUE;
    }
    else
    {
        // Compare the change of data to threshold, and only push the data back to
        // clx if the change exceeds threshold. This is usually done in HW.
        if ( (abs(m_CachedData.Axis.X - m_LastSample.Axis.X) >= m_CachedThresholds.Axis.X) ||
             (abs(m_CachedData.Axis.Y - m_LastSample.Axis.Y) >= m_CachedThresholds.Axis.Y) ||
             (abs(m_CachedData.Axis.Z - m_LastSample.Axis.Z) >= m_CachedThresholds.Axis.Z))
        {
            DataReady = TRUE;
        }
    }

    if (DataReady != FALSE)
    {
        // update last sample
        m_LastSample.Axis.X = m_CachedData.Axis.X;
        m_LastSample.Axis.Y = m_CachedData.Axis.Y;
        m_LastSample.Axis.Z = m_CachedData.Axis.Z;

        m_LastSample.Shake = m_CachedData.Shake;

        // push to clx
        InitPropVariantFromFloat(m_LastSample.Axis.X, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_X].Value));
        InitPropVariantFromFloat(m_LastSample.Axis.Y, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_Y].Value));
        InitPropVariantFromFloat(m_LastSample.Axis.Z, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_Z].Value));

        InitPropVariantFromBoolean(m_LastSample.Shake, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_SHAKE].Value));

        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[LINEAR_ACCELEROMETER_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("COMBO %!FUNC! LAC Data did NOT meet the threshold");
    }

    SENSOR_FunctionExit(Status);
    return Status;
}



//------------------------------------------------------------------------------
// Function: UpdateCachedThreshold
//
// This routine updates the cached threshold
//
// Arguments:
//       None
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
LinearAccelerometerDevice::UpdateCachedThreshold(
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AccelerationX_Gs,
                                    &m_CachedThresholds.Axis.X);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! LAC PropKeyFindKeyGetFloat for X failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AccelerationY_Gs,
                                    &m_CachedThresholds.Axis.Y);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! LAC PropKeyFindKeyGetFloat for Y failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AccelerationZ_Gs,
                                    &m_CachedThresholds.Axis.Z);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! LAC PropKeyFindKeyGetFloat for Z failed! %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}