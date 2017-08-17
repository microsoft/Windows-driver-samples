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

#include "RelativeFusionClient.tmh"

#define SENSORV2_POOL_TAG_RELATIVE_FUSION           '2FsR'

#define RelativeFusion_Initial_MinDataInterval_Ms   (10)          // 100Hz

#define RelativeFusion_Quarternion_Maximum          (1.0f)
#define RelativeFusion_Quarternion_Minimum          (-1.0f)
#define RelativeFusion_Quarternion_Resolution       ((RelativeFusion_Quarternion_Maximum-RelativeFusion_Quarternion_Minimum)/65536)

// Relative Fusion Sensor Unique ID
// {0D324DDF-05AD-4285-875A-B2BCF565D2BA}
DEFINE_GUID(GUID_RelativeFusionDevice_UniqueID,
    0xd324ddf, 0x5ad, 0x4285, 0x87, 0x5a, 0xb2, 0xbc, 0xf5, 0x65, 0xd2, 0xba);


// Sensor data
typedef enum
{
    RELATIVE_FUSION_DATA_TIMESTAMP = 0,
    RELATIVE_FUSION_DATA_QUATERNION_W,
    RELATIVE_FUSION_DATA_QUATERNION_X,
    RELATIVE_FUSION_DATA_QUATERNION_Y,
    RELATIVE_FUSION_DATA_QUATERNION_Z,
    RELATIVE_FUSION_DATA_COUNT
} RELATIVE_FUSION_DATA_INDEX;

// Sensor thresholds
typedef enum
{
    RELATIVE_FUSION_THRESHOLD_QUATERNION_W = 0,
    RELATIVE_FUSION_THRESHOLD_QUATERNION_X,
    RELATIVE_FUSION_THRESHOLD_QUATERNION_Y,
    RELATIVE_FUSION_THRESHOLD_QUATERNION_Z,
    RELATIVE_FUSION_THRESHOLD_COUNT
} RELATIVE_FUSION_THRESHOLD_INDEX;



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
RelativeFusionDevice::Initialize(
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
        TraceError("COMBO %!FUNC! Relative Fusion WdfWaitLockCreate failed %!STATUS!", Status);
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

        Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_Timer);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! RelativeFusionDevice WdfTimerCreate failed %!STATUS!", Status);
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
                                 SENSORV2_POOL_TAG_RELATIVE_FUSION,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pEnumerationProperties));
        if (!NT_SUCCESS(Status) || nullptr == m_pEnumerationProperties)
        {
            TraceError("COMBO %!FUNC! RelativeFusionDevice WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
        m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_RelativeOrientation,
                                 &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"Manufacturer name",
                                  &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"Relative Fusion Device",
                                  &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Key = DEVPKEY_Sensor_ConnectionType;
        // The DEVPKEY_Sensor_ConnectionType values match the SensorConnectionType enumeration
        InitPropVariantFromUInt32(static_cast<ULONG>(SensorConnectionType::Integrated),
                                 &(m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_RelativeFusionDevice_UniqueID,
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
        ULONG Size = SENSOR_PROPERTY_LIST_SIZE(RELATIVE_FUSION_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_RELATIVE_FUSION,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pSupportedDataFields));
        if (!NT_SUCCESS(Status) || nullptr == m_pSupportedDataFields)
        {
            TraceError("COMBO %!FUNC! RelativeFusionDevice WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
        m_pSupportedDataFields->Count = RELATIVE_FUSION_DATA_COUNT;

        m_pSupportedDataFields->List[RELATIVE_FUSION_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[RELATIVE_FUSION_DATA_QUATERNION_W] = PKEY_SensorData_QuaternionW;
        m_pSupportedDataFields->List[RELATIVE_FUSION_DATA_QUATERNION_X] = PKEY_SensorData_QuaternionX;
        m_pSupportedDataFields->List[RELATIVE_FUSION_DATA_QUATERNION_Y] = PKEY_SensorData_QuaternionY;
        m_pSupportedDataFields->List[RELATIVE_FUSION_DATA_QUATERNION_Z] = PKEY_SensorData_QuaternionZ;
    }

    //
    // Data
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(RELATIVE_FUSION_DATA_COUNT);
        FILETIME Time = {0};

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_RELATIVE_FUSION,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pData));
        if (!NT_SUCCESS(Status) || nullptr == m_pData)
        {
            TraceError("COMBO %!FUNC! RelativeFusionDevice WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
        m_pData->Count = RELATIVE_FUSION_DATA_COUNT;

        m_pData->List[RELATIVE_FUSION_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
        GetSystemTimePreciseAsFileTime(&Time);
        InitPropVariantFromFileTime(&Time, &(m_pData->List[RELATIVE_FUSION_DATA_TIMESTAMP].Value));

        m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_W].Key = PKEY_SensorData_QuaternionW;
        InitPropVariantFromFloat(0.0, &(m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_W].Value));

        m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_X].Key = PKEY_SensorData_QuaternionX;
        InitPropVariantFromFloat(0.0, &(m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_X].Value));

        m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_Y].Key = PKEY_SensorData_QuaternionY;
        InitPropVariantFromFloat(0.0, &(m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_Y].Value));

        m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_Z].Key = PKEY_SensorData_QuaternionZ;
        InitPropVariantFromFloat(0.0, &(m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_Z].Value));

        ZeroMemory(&m_LastSample, sizeof(m_LastSample));
        ZeroMemory(&m_CachedData, sizeof(m_CachedData));
        m_CachedData.W = 1.0f; // This is the initial position
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
                                 SENSORV2_POOL_TAG_RELATIVE_FUSION,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pProperties));
        if (!NT_SUCCESS(Status) || nullptr == m_pProperties)
        {
            TraceError("COMBO %!FUNC! RelativeFusionDevice WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
        m_pProperties->Count = SENSOR_COMMON_PROPERTY_COUNT;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(RelativeFusion_Initial_MinDataInterval_Ms,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Value));
        m_IntervalMs = RelativeFusion_Initial_MinDataInterval_Ms;
        m_MinimumIntervalMs = RelativeFusion_Initial_MinDataInterval_Ms;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_RelativeOrientation,
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
                                 SENSORV2_POOL_TAG_RELATIVE_FUSION,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pDataFieldProperties));
        if (!NT_SUCCESS(Status) || nullptr == m_pDataFieldProperties)
        {
            TraceError("COMBO %!FUNC! RelativeFusionDevice WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
        m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

        m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat(RelativeFusion_Quarternion_Resolution,
                                 &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

        m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(RelativeFusion_Quarternion_Minimum,
                                 &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

        m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(RelativeFusion_Quarternion_Maximum,
                                 &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));
    }

    //
    // Set default threshold
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size =  SENSOR_COLLECTION_LIST_SIZE(RELATIVE_FUSION_THRESHOLD_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_RELATIVE_FUSION,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pThresholds));
        if (!NT_SUCCESS(Status) || nullptr == m_pThresholds)
        {
            TraceError("COMBO %!FUNC! RelativeFusionDevice WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
        m_pThresholds->Count = RELATIVE_FUSION_THRESHOLD_COUNT;

        m_pThresholds->List[RELATIVE_FUSION_THRESHOLD_QUATERNION_W].Key = PKEY_SensorData_QuaternionW;
        InitPropVariantFromFloat(0.0f, &(m_pThresholds->List[RELATIVE_FUSION_THRESHOLD_QUATERNION_W].Value));

        m_pThresholds->List[RELATIVE_FUSION_THRESHOLD_QUATERNION_X].Key = PKEY_SensorData_QuaternionX;
        InitPropVariantFromFloat(0.0f, &(m_pThresholds->List[RELATIVE_FUSION_THRESHOLD_QUATERNION_X].Value));

        m_pThresholds->List[RELATIVE_FUSION_THRESHOLD_QUATERNION_Y].Key = PKEY_SensorData_QuaternionY;
        InitPropVariantFromFloat(0.0f, &(m_pThresholds->List[RELATIVE_FUSION_THRESHOLD_QUATERNION_Y].Value));

        m_pThresholds->List[RELATIVE_FUSION_THRESHOLD_QUATERNION_Z].Key = PKEY_SensorData_QuaternionZ;
        InitPropVariantFromFloat(0.0f, &(m_pThresholds->List[RELATIVE_FUSION_THRESHOLD_QUATERNION_Z].Value));

        ZeroMemory(&m_CachedThresholds, sizeof(m_CachedThresholds));
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
RelativeFusionDevice::GetData(
    )
{
    BOOLEAN DataReady = FALSE;
    FILETIME TimeStamp = {0};
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // new sample?
    if (FALSE != m_FirstSample)
    {
        Status = GetPerformanceTime (&m_StartTime);
        if (!NT_SUCCESS(Status))
        {
            m_StartTime = 0;
            TraceError("COMBO %!FUNC! RelativeFusionDevice GetPerformanceTime %!STATUS!", Status);
        }

        m_SampleCount = 0;

        DataReady = TRUE;
    }
    else
    {
        // Compare the change of data to threshold, and only push the data back to
        // clx if the change exceeds threshold. This is usually done in HW.
        if ((abs(m_CachedData.W - m_LastSample.W) >= m_CachedThresholds.W) ||
            (abs(m_CachedData.X - m_LastSample.X) >= m_CachedThresholds.X) ||
            (abs(m_CachedData.Y - m_LastSample.Y) >= m_CachedThresholds.Y) ||
            (abs(m_CachedData.Z - m_LastSample.Z) >= m_CachedThresholds.Z))
        {
            DataReady = TRUE;
        }
    }

    if (FALSE != DataReady)
    {
        // update last sample
        m_LastSample = m_CachedData;

        // push to clx
        InitPropVariantFromFloat(m_LastSample.W, &(m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_W].Value));
        InitPropVariantFromFloat(m_LastSample.X, &(m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_X].Value));
        InitPropVariantFromFloat(m_LastSample.Y, &(m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_Y].Value));
        InitPropVariantFromFloat(m_LastSample.Z, &(m_pData->List[RELATIVE_FUSION_DATA_QUATERNION_Z].Value));


        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[RELATIVE_FUSION_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("COMBO %!FUNC! RelativeFusionDevice Data did NOT meet the threshold");
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
RelativeFusionDevice::UpdateCachedThreshold(
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    SENSOR_FunctionEnter();

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_QuaternionW,
                                    &m_CachedThresholds.W);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! RelativeFusionDevice PropKeyFindKeyGetFloat for W failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_QuaternionX,
                                    &m_CachedThresholds.X);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! RelativeFusionDevice PropKeyFindKeyGetFloat for X failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_QuaternionY,
                                    &m_CachedThresholds.Y);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! RelativeFusionDevice PropKeyFindKeyGetFloat for Y failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_QuaternionZ,
                                    &m_CachedThresholds.Z);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! RelativeFusionDevice PropKeyFindKeyGetFloat for Z failed! %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}