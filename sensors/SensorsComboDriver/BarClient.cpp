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

#include "BarClient.tmh"

#define SENSORV2_POOL_TAG_BAROMETER               '2RaB'

#define Bar_Initial_MinDataInterval_Ms            (10)          // 100Hz
#define Bar_Initial_Threshold_Bar                 (0.001f)      // 1 mBar ~= 10 meter

#define BarDevice_Minimum_Bar                     (0.3f)
#define BarDevice_Maximum_Bar                     (1.1f)
#define BarDevice_Precision                       (65536.0f)    // 65536 = 2^16, 16 bit data
#define BarDevice_Range_Bar                       (BarDevice_Maximum_Bar - BarDevice_Minimum_Bar)
#define BarDevice_Resolution_Bar                  (BarDevice_Range_Bar / BarDevice_Precision)

// Barometer Unique ID
// {46CB48CE-272D-4402-8E09-07748F8940CA}
DEFINE_GUID(GUID_BarDevice_UniqueID,
    0x46cb48ce, 0x272d, 0x4402, 0x8e, 0x9, 0x7, 0x74, 0x8f, 0x89, 0x40, 0xca);

// Sensor data
typedef enum
{
    BAR_DATA_TIMESTAMP = 0,
    BAR_DATA_PRESSURE,
    BAR_DATA_COUNT
} BAR_DATA_INDEX;

// Sensor thresholds
typedef enum
{
    BAR_THRESHOLD_PRESSURE = 0,
    BAR_THRESHOLD_COUNT
} BAR_THRESHOLD_INDEX;



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
BarDevice::Initialize(
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
        TraceError("COMBO %!FUNC! BAR WdfWaitLockCreate failed %!STATUS!", Status);
    }

    //
    // Create timer object for polling sensor samples
    //
    if (NT_SUCCESS(Status))
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
            TraceError("COMBO %!FUNC! BAR WdfTimerCreate failed %!STATUS!", Status);
        }
    }

    //
    // Sensor Enumeration Properties
    //
    if (NT_SUCCESS(Status))
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ENUMERATION_PROPERTIES_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_BAROMETER,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pEnumerationProperties));
        if (!NT_SUCCESS(Status) || nullptr == m_pEnumerationProperties)
        {
            TraceError("COMBO %!FUNC! BAR WdfMemoryCreate failed %!STATUS!", Status);
        }
        else
        {
            SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
            m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

            m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
            InitPropVariantFromCLSID(GUID_SensorType_Barometer,
                                     &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

            m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
            InitPropVariantFromString(L"Manufacturer name",
                                      &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

            m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
            InitPropVariantFromString(L"Barometer",
                                      &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

            m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Key = DEVPKEY_Sensor_ConnectionType;
            // The DEVPKEY_Sensor_ConnectionType values match the SensorConnectionType enumeration
            InitPropVariantFromUInt32(static_cast<ULONG>(SensorConnectionType::Integrated),
                                     &(m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Value));

            m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
            InitPropVariantFromCLSID(GUID_BarDevice_UniqueID,
                                     &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));

            m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Key = DEVPKEY_Sensor_IsPrimary;
            InitPropVariantFromBoolean(FALSE,
                                     &(m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Value));
        }
    }

    //
    // Supported Data-Fields
    //
    if (NT_SUCCESS(Status))
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_PROPERTY_LIST_SIZE(BAR_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_BAROMETER,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pSupportedDataFields));
        if (!NT_SUCCESS(Status) || nullptr == m_pSupportedDataFields)
        {
            TraceError("COMBO %!FUNC! BAR WdfMemoryCreate failed %!STATUS!", Status);
        }
        else
        {
            SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
            m_pSupportedDataFields->Count = BAR_DATA_COUNT;

            m_pSupportedDataFields->List[BAR_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
            m_pSupportedDataFields->List[BAR_DATA_PRESSURE] = PKEY_SensorData_AtmosphericPressure_Bars;
        }
    }

    //
    // Data
    //
    if (NT_SUCCESS(Status))
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(BAR_DATA_COUNT);
        FILETIME Time = {};

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_BAROMETER,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pData));
        if (!NT_SUCCESS(Status) || nullptr == m_pData)
        {
            TraceError("COMBO %!FUNC! BAR WdfMemoryCreate failed %!STATUS!", Status);
        }
        else
        {
            SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
            m_pData->Count = BAR_DATA_COUNT;

            m_pData->List[BAR_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
            GetSystemTimePreciseAsFileTime(&Time);
            InitPropVariantFromFileTime(&Time, &(m_pData->List[BAR_DATA_TIMESTAMP].Value));

            m_pData->List[BAR_DATA_PRESSURE].Key = PKEY_SensorData_AtmosphericPressure_Bars;
            InitPropVariantFromFloat(0.0f, &(m_pData->List[BAR_DATA_PRESSURE].Value));

            m_CachedData = 1.013f;
            m_LastSample = 0.0f;
        }
    }

    //
    // Sensor Properties
    //
    if (NT_SUCCESS(Status))
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_COMMON_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_BAROMETER,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pProperties));
        if (!NT_SUCCESS(Status) || nullptr == m_pProperties)
        {
            TraceError("COMBO %!FUNC! BAR WdfMemoryCreate failed %!STATUS!", Status);
        }
        else
        {
            SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
            m_pProperties->Count = SENSOR_COMMON_PROPERTY_COUNT;

            m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Key = PKEY_Sensor_State;
            InitPropVariantFromUInt32(SensorState_Initializing,
                                      &(m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

            m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
            InitPropVariantFromUInt32(Bar_Initial_MinDataInterval_Ms,
                                      &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Value));
            m_IntervalMs = Bar_Initial_MinDataInterval_Ms;
            m_MinimumIntervalMs = Bar_Initial_MinDataInterval_Ms;

            m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
            InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
                                      &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Value));

            m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Key = PKEY_Sensor_Type;
            InitPropVariantFromCLSID(GUID_SensorType_Barometer,
                                     &(m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Value));
        }
    }

    //
    // Data filed properties
    //
    if (NT_SUCCESS(Status))
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_DATA_FIELD_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_BAROMETER,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pDataFieldProperties));
        if (!NT_SUCCESS(Status) || nullptr == m_pDataFieldProperties)
        {
            TraceError("COMBO %!FUNC! BAR WdfMemoryCreate failed %!STATUS!", Status);
        }
        else
        {
            SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
            m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

            m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
            InitPropVariantFromFloat(BarDevice_Resolution_Bar,
                                     &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

            m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
            InitPropVariantFromFloat(BarDevice_Minimum_Bar,
                                     &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

            m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
            InitPropVariantFromFloat(BarDevice_Maximum_Bar,
                                     &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));
        }
    }

    //
    // Set default threshold
    //
    if (NT_SUCCESS(Status))
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size =  SENSOR_COLLECTION_LIST_SIZE(BAR_THRESHOLD_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_BAROMETER,
                                 Size,
                                 &MemoryHandle,
                                 reinterpret_cast<PVOID*>(&m_pThresholds));
        if (!NT_SUCCESS(Status) || nullptr == m_pThresholds)
        {
            TraceError("COMBO %!FUNC! BAR WdfMemoryCreate failed %!STATUS!", Status);
        }
        else
        {
            SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
            m_pThresholds->Count = BAR_THRESHOLD_COUNT;

            m_pThresholds->List[BAR_THRESHOLD_PRESSURE].Key = PKEY_SensorData_AtmosphericPressure_Bars;
            InitPropVariantFromFloat(Bar_Initial_Threshold_Bar,
                                     &(m_pThresholds->List[BAR_THRESHOLD_PRESSURE].Value));

            m_CachedThresholds = Bar_Initial_Threshold_Bar;
            m_FirstSample = TRUE;
        }
    }

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
BarDevice::GetData(
    )
{
    BOOLEAN DataReady = FALSE;
    FILETIME TimeStamp = {};
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // new sample?
    if (FALSE != m_FirstSample)
    {
        Status = GetPerformanceTime (&m_StartTime);
        if (!NT_SUCCESS(Status))
        {
            m_StartTime = 0;
            TraceError("COMBO %!FUNC! BAR GetPerformanceTime %!STATUS!", Status);
        }

        m_SampleCount = 0;

        DataReady = TRUE;
    }
    else
    {
        // Compare the change of data to threshold, and only push the data back to
        // clx if the change exceeds threshold. This is usually done in HW.
        if ((abs(m_CachedData - m_LastSample) >= m_CachedThresholds))
        {
            DataReady = TRUE;
        }
    }

    if (FALSE != DataReady)
    {
        // update last sample
        m_LastSample = m_CachedData;

        // push to clx
        InitPropVariantFromFloat(m_LastSample, &(m_pData->List[BAR_DATA_PRESSURE].Value));

        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[BAR_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("COMBO %!FUNC! BAR Data did NOT meet the threshold");
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
BarDevice::UpdateCachedThreshold(
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    SENSOR_FunctionEnter();

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_AtmosphericPressure_Bars,
                                    &m_CachedThresholds);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! BAR PropKeyFindKeyGetFloat for pressure failed %!STATUS!", Status);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}