// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//
//  This module contains the implementation of sensor specific functions.
//
// Environment:
//
//  Windows User-Mode Driver Framework (WUDF)

#include "Clients.h"

#include "GyrClient.tmh"

#define SENSORV2_POOL_TAG_GYROSCOPE              '2oyG'

// Chasis says 5-250Hz but unit test fails if below 250Hz???
#define Gyr_MinDataInterval_Ms                 (4)        // 250Hz
#define Gyr_Initial_Threshold_DegreesPerSecond (10.0f)

#define GyrDevice_Minimum_DegreesPerSecond     (-2000.0f)
#define GyrDevice_Maximum_DegreesPerSecond     (2000.0f)
#define GyrDevice_Precision                    (65536.0f) // 65536 = 2^16, 16 bit data
#define GyrDevice_Range_DegreesPerSecond      \
            (GyrDevice_Maximum_DegreesPerSecond - GyrDevice_Minimum_DegreesPerSecond)
#define GyrDevice_Resolution_DegreesPerSecond \
            (GyrDevice_Range_DegreesPerSecond / GyrDevice_Precision)

//  Gyroscope Unique ID
// {61A61B96-1E4C-47C6-8697-654680101446}
DEFINE_GUID(GUID_GyrDevice_UniqueID,
    0x61a61b96, 0x1e4c, 0x47c6, 0x86, 0x97, 0x65, 0x46, 0x80, 0x10, 0x14, 0x46);

// Sensor data
typedef enum
{
    GYR_DATA_TIMESTAMP = 0,
    GYR_DATA_X,
    GYR_DATA_Y,
    GYR_DATA_Z,
    GYR_DATA_COUNT
} GYR_DATA_INDEX;

// Sensor thresholds
typedef enum
{
    GYR_THRESHOLD_X = 0,
    GYR_THRESHOLD_Y,
    GYR_THRESHOLD_Z,
    GYR_THRESHOLD_COUNT
} GYR_THRESHOLD_INDEX;

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
GyrDevice::Initialize(
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
        TraceError("COMBO %!FUNC! GYR WdfWaitLockCreate failed %!STATUS!", Status);
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
            TraceError("COMBO %!FUNC! GYR WdfTimerCreate failed %!STATUS!", Status);
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
                                 SENSORV2_POOL_TAG_GYROSCOPE,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pEnumerationProperties);
        if (!NT_SUCCESS(Status) || m_pEnumerationProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! GYR WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
        m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_Gyrometer3D,
                                 &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"Manufacturer name",
                                  &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"GYR",
                                  &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Key = DEVPKEY_Sensor_ConnectionType;
        // The DEVPKEY_Sensor_ConnectionType values match the SensorConnectionType enumeration
        InitPropVariantFromUInt32(static_cast<ULONG>(SensorConnectionType::Integrated),
                                 &(m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_GyrDevice_UniqueID,
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
        ULONG Size = SENSOR_PROPERTY_LIST_SIZE(GYR_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_GYROSCOPE,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pSupportedDataFields);
        if (!NT_SUCCESS(Status) || m_pSupportedDataFields == nullptr)
        {
            TraceError("COMBO %!FUNC! GYR WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
        m_pSupportedDataFields->Count = GYR_DATA_COUNT;

        m_pSupportedDataFields->List[GYR_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[GYR_DATA_X] = PKEY_SensorData_AngularVelocityX_DegreesPerSecond;
        m_pSupportedDataFields->List[GYR_DATA_Y] = PKEY_SensorData_AngularVelocityY_DegreesPerSecond;
        m_pSupportedDataFields->List[GYR_DATA_Z] = PKEY_SensorData_AngularVelocityZ_DegreesPerSecond;
    }

    //
    // Data
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(GYR_DATA_COUNT);
        FILETIME Time = {0};

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_GYROSCOPE,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pData);
        if (!NT_SUCCESS(Status) || m_pData == nullptr)
        {
            TraceError("COMBO %!FUNC! GYR WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
        m_pData->Count = GYR_DATA_COUNT;

        m_pData->List[GYR_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
        GetSystemTimePreciseAsFileTime(&Time);
        InitPropVariantFromFileTime(&Time, &(m_pData->List[GYR_DATA_TIMESTAMP].Value));

        m_pData->List[GYR_DATA_X].Key = PKEY_SensorData_AngularVelocityX_DegreesPerSecond;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[GYR_DATA_X].Value));

        m_pData->List[GYR_DATA_Y].Key = PKEY_SensorData_AngularVelocityY_DegreesPerSecond;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[GYR_DATA_Y].Value));

        m_pData->List[GYR_DATA_Z].Key = PKEY_SensorData_AngularVelocityZ_DegreesPerSecond;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[GYR_DATA_Z].Value));

        m_CachedData.X = 0.0f;
        m_CachedData.Y = 0.0f;
        m_CachedData.Z = 0.0f;

        m_LastSample.X = 0.0f;
        m_LastSample.Y = 0.0f;
        m_LastSample.Z = 0.0f;
    }

    //
    // Sensor Properties
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_COMMON_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_GYROSCOPE,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pProperties);
        if (!NT_SUCCESS(Status) || m_pProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! GYR WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
        m_pProperties->Count = SENSOR_COMMON_PROPERTY_COUNT;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(Gyr_MinDataInterval_Ms,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Value));
        m_IntervalMs = Gyr_MinDataInterval_Ms;
        m_MinimumIntervalMs = Gyr_MinDataInterval_Ms;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_Gyrometer3D,
                                 &(m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Value));
    }

    //
    // Data filed properties
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
                                 SENSORV2_POOL_TAG_GYROSCOPE,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pDataFieldProperties);
        if (!NT_SUCCESS(Status) || m_pDataFieldProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! GYR WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
        m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

        m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat(GyrDevice_Resolution_DegreesPerSecond,
                                 &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

        m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(GyrDevice_Minimum_DegreesPerSecond,
                                 &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

        m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(GyrDevice_Maximum_DegreesPerSecond,
                                 &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));
    }

    //
    // Set default threshold
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size =  SENSOR_COLLECTION_LIST_SIZE(GYR_THRESHOLD_COUNT);    //  Timestamp and shake do not have thresholds

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_GYROSCOPE,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pThresholds);
        if (!NT_SUCCESS(Status) || m_pThresholds == nullptr)
        {
            TraceError("COMBO %!FUNC! GYR WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
        m_pThresholds->Count = GYR_THRESHOLD_COUNT;

        m_pThresholds->List[GYR_THRESHOLD_X].Key = PKEY_SensorData_AngularVelocityX_DegreesPerSecond;
        InitPropVariantFromFloat(Gyr_Initial_Threshold_DegreesPerSecond,
                                    &(m_pThresholds->List[GYR_THRESHOLD_X].Value));

        m_pThresholds->List[GYR_THRESHOLD_Y].Key = PKEY_SensorData_AngularVelocityY_DegreesPerSecond;
        InitPropVariantFromFloat(Gyr_Initial_Threshold_DegreesPerSecond,
                                    &(m_pThresholds->List[GYR_THRESHOLD_Y].Value));

        m_pThresholds->List[GYR_THRESHOLD_Z].Key = PKEY_SensorData_AngularVelocityZ_DegreesPerSecond;
        InitPropVariantFromFloat(Gyr_Initial_Threshold_DegreesPerSecond,
                                    &(m_pThresholds->List[GYR_THRESHOLD_Z].Value));

        m_CachedThresholds.X = Gyr_Initial_Threshold_DegreesPerSecond;
        m_CachedThresholds.Y = Gyr_Initial_Threshold_DegreesPerSecond;
        m_CachedThresholds.Z = Gyr_Initial_Threshold_DegreesPerSecond;

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
GyrDevice::GetData(
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
            TraceError("COMBO %!FUNC! GYR GetPerformanceTime %!STATUS!", Status);
        }

        m_SampleCount = 0;

        DataReady = TRUE;
    }
    else
    {
        // Compare the change of data to threshold, and only push the data back to
        // clx if the change exceeds threshold. This is usually done in HW.
        if ( (abs(m_CachedData.X - m_LastSample.X) >= m_CachedThresholds.X) ||
             (abs(m_CachedData.Y - m_LastSample.Y) >= m_CachedThresholds.Y) ||
             (abs(m_CachedData.Z - m_LastSample.Z) >= m_CachedThresholds.Z))
        {
            DataReady = TRUE;
        }
    }

    if (DataReady != FALSE)
    {
        // update last sample
        m_LastSample.X = m_CachedData.X;
        m_LastSample.Y = m_CachedData.Y;
        m_LastSample.Z = m_CachedData.Z;

        // push to clx
        InitPropVariantFromFloat(m_LastSample.X, &(m_pData->List[GYR_DATA_X].Value));
        InitPropVariantFromFloat(m_LastSample.Y, &(m_pData->List[GYR_DATA_Y].Value));
        InitPropVariantFromFloat(m_LastSample.Z, &(m_pData->List[GYR_DATA_Z].Value));

        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[GYR_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("COMBO %!FUNC! GYR Data did NOT meet the threshold");
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
GyrDevice::UpdateCachedThreshold(
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AngularVelocityX_DegreesPerSecond,
                                    &m_CachedThresholds.X);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! GYR PropKeyFindKeyGetFloat for X failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AngularVelocityY_DegreesPerSecond,
                                    &m_CachedThresholds.Y);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! GYR PropKeyFindKeyGetFloat for Y failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AngularVelocityZ_DegreesPerSecond,
                                    &m_CachedThresholds.Z);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! GYR PropKeyFindKeyGetFloat for Z failed! %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}