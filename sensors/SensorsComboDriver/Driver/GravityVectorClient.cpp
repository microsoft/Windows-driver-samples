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

#include "GravityVectorClient.tmh"



#define SENSORV2_POOL_TAG_GRAVITY_VECTOR            '6ArG'

#define GravityVectorDevice_Default_MinDataInterval (4)
#define GravityVectorDevice_Default_Axis_Threshold  (1.0f)
#define GravityVectorDevice_Axis_Resolution         (4.0f / 65536.0f) // in delta g
#define GravityVectorDevice_Axis_Minimum            (-2.0f)           // in g
#define GravityVectorDevice_Axis_Maximum            (2.0f)            // in g


//  Linear Accelerometer Unique ID
// {56E11473-2BB7-41E4-863A-DA014200C0DC}
DEFINE_GUID(GUID_GravityVectorDevice_UniqueID,
    0x56e11473, 0x2bb7, 0x41e4, 0x86, 0x3a, 0xda, 0x1, 0x42, 0x0, 0xc0, 0xdc);

// Sensor data
typedef enum
{
    GRAVITY_VECTOR_DATA_X = 0,
    GRAVITY_VECTOR_DATA_Y,
    GRAVITY_VECTOR_DATA_Z,
    GRAVITY_VECTOR_DATA_TIMESTAMP,
    GRAVITY_VECTOR_DATA_COUNT
} GRAVITY_VECTOR_DATA_INDEX;

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
GravityVectorDevice::Initialize(
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
        TraceError("COMBO %!FUNC! GRA WdfWaitLockCreate failed %!STATUS!", Status);
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
            TraceError("COMBO %!FUNC! GRA WdfTimerCreate failed %!STATUS!", Status);
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
                                 SENSORV2_POOL_TAG_GRAVITY_VECTOR,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pEnumerationProperties);
        if (!NT_SUCCESS(Status) || m_pEnumerationProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! GRA WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
        m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_GravityVector,
                                 &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"Manufacturer name",
                                  &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"Gravity Vector Sensor",
                                  &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Key = DEVPKEY_Sensor_ConnectionType;
        // The DEVPKEY_Sensor_ConnectionType values match the SensorConnectionType enumeration
        InitPropVariantFromUInt32(static_cast<ULONG>(SensorConnectionType::Integrated),
                                 &(m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_GravityVectorDevice_UniqueID,
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
        ULONG Size = SENSOR_PROPERTY_LIST_SIZE(GRAVITY_VECTOR_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_GRAVITY_VECTOR,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pSupportedDataFields);
        if (!NT_SUCCESS(Status) || m_pSupportedDataFields == nullptr)
        {
            TraceError("COMBO %!FUNC! GRA WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
        m_pSupportedDataFields->Count = GRAVITY_VECTOR_DATA_COUNT;

        m_pSupportedDataFields->List[GRAVITY_VECTOR_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[GRAVITY_VECTOR_DATA_X] = PKEY_SensorData_AccelerationX_Gs;
        m_pSupportedDataFields->List[GRAVITY_VECTOR_DATA_Y] = PKEY_SensorData_AccelerationY_Gs;
        m_pSupportedDataFields->List[GRAVITY_VECTOR_DATA_Z] = PKEY_SensorData_AccelerationZ_Gs;
    }

    //
    // Data
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(GRAVITY_VECTOR_DATA_COUNT);
        FILETIME Time = {0};

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_GRAVITY_VECTOR,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pData);
        if (!NT_SUCCESS(Status) || m_pData == nullptr)
        {
            TraceError("COMBO %!FUNC! GRA WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

    SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
    m_pData->Count = GRAVITY_VECTOR_DATA_COUNT;

    m_pData->List[GRAVITY_VECTOR_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
    GetSystemTimePreciseAsFileTime(&Time);
    InitPropVariantFromFileTime(&Time, &(m_pData->List[GRAVITY_VECTOR_DATA_TIMESTAMP].Value));

    m_pData->List[GRAVITY_VECTOR_DATA_X].Key = PKEY_SensorData_AccelerationX_Gs;
    InitPropVariantFromFloat(0.0, &(m_pData->List[GRAVITY_VECTOR_DATA_X].Value));

    m_pData->List[GRAVITY_VECTOR_DATA_Y].Key = PKEY_SensorData_AccelerationY_Gs;
    InitPropVariantFromFloat(0.0, &(m_pData->List[GRAVITY_VECTOR_DATA_Y].Value));

    m_pData->List[GRAVITY_VECTOR_DATA_Z].Key = PKEY_SensorData_AccelerationZ_Gs;
    InitPropVariantFromFloat(0.0, &(m_pData->List[GRAVITY_VECTOR_DATA_Z].Value));

    m_CachedData.X = 0.0f;
    m_CachedData.Y = 0.0f;
    m_CachedData.Z = 0.0f;

    m_LastSample.X  = 0.0f;
    m_LastSample.Y  = 0.0f;
    m_LastSample.Z  = 0.0f;
    }

    //
    // Sensor Properties
    //
    {
        m_IntervalMs = GravityVectorDevice_Default_MinDataInterval;

        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_COMMON_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_GRAVITY_VECTOR,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pProperties);
        if (!NT_SUCCESS(Status) || m_pProperties == nullptr)
        {
            TraceError("GRA %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
        m_pProperties->Count = SENSOR_COMMON_PROPERTY_COUNT;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(GravityVectorDevice_Default_MinDataInterval,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_GravityVector,
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
                                 SENSORV2_POOL_TAG_GRAVITY_VECTOR,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pDataFieldProperties);
        if (!NT_SUCCESS(Status) || m_pDataFieldProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! GRA WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
        m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

        m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat(GravityVectorDevice_Axis_Resolution,
                                 &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

        m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(GravityVectorDevice_Axis_Minimum,
                                 &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

        m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(GravityVectorDevice_Axis_Maximum,
                                 &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));
    }

    //
    // Set default threshold
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;

        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(GRAVITY_VECTOR_DATA_COUNT - 1);    //  Timestamp does not have thresholds

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_GRAVITY_VECTOR,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pThresholds);
        if (!NT_SUCCESS(Status) || m_pThresholds == nullptr)
        {
            TraceError("COMBO %!FUNC! GRA WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
        m_pThresholds->Count = GRAVITY_VECTOR_DATA_COUNT - 1;

        m_pThresholds->List[GRAVITY_VECTOR_DATA_X].Key = PKEY_SensorData_AccelerationX_Gs;
        InitPropVariantFromFloat(GravityVectorDevice_Default_Axis_Threshold,
                                 &(m_pThresholds->List[GRAVITY_VECTOR_DATA_X].Value));

        m_pThresholds->List[GRAVITY_VECTOR_DATA_Y].Key = PKEY_SensorData_AccelerationY_Gs;
        InitPropVariantFromFloat(GravityVectorDevice_Default_Axis_Threshold,
                                 &(m_pThresholds->List[GRAVITY_VECTOR_DATA_Y].Value));

        m_pThresholds->List[GRAVITY_VECTOR_DATA_Z].Key = PKEY_SensorData_AccelerationZ_Gs;
        InitPropVariantFromFloat(GravityVectorDevice_Default_Axis_Threshold,
                                 &(m_pThresholds->List[GRAVITY_VECTOR_DATA_Z].Value));

        m_CachedThresholds.X = GravityVectorDevice_Default_Axis_Threshold;
        m_CachedThresholds.Y = GravityVectorDevice_Default_Axis_Threshold;
        m_CachedThresholds.Z = GravityVectorDevice_Default_Axis_Threshold;

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
GravityVectorDevice::GetData(
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
            TraceError("COMBO %!FUNC! GRA GetPerformanceTime %!STATUS!", Status);
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
        InitPropVariantFromFloat(m_LastSample.X, &(m_pData->List[GRAVITY_VECTOR_DATA_X].Value));
        InitPropVariantFromFloat(m_LastSample.Y, &(m_pData->List[GRAVITY_VECTOR_DATA_Y].Value));
        InitPropVariantFromFloat(m_LastSample.Z, &(m_pData->List[GRAVITY_VECTOR_DATA_Z].Value));

        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[GRAVITY_VECTOR_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("COMBO %!FUNC! GRA Data did NOT meet the threshold");
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
GravityVectorDevice::UpdateCachedThreshold(
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AccelerationX_Gs,
                                    &m_CachedThresholds.X);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! GRA PropKeyFindKeyGetFloat for X failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AccelerationY_Gs,
                                    &m_CachedThresholds.Y);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! GRA PropKeyFindKeyGetFloat for Y failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AccelerationZ_Gs,
                                    &m_CachedThresholds.Z);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! GRA PropKeyFindKeyGetFloat for Z failed! %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}