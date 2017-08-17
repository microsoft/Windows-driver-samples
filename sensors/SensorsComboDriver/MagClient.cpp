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

#include "MagClient.tmh"

#define SENSORV2_POOL_TAG_MAGNETOMETER       '2ngM'

#define Mag_Initial_MinDataInterval_Ms    (10)       // 100Hz
#define Mag_Initial_Threshold_Microteslas (5.0f)     // 5uT

#define MagDevice_Minimum_Microteslas     (-2000.0f)
#define MagDevice_Maximum_Microteslas     (2000.0f)
#define MagDevice_Precision               (65536.0f) // 65536 = 2^16, 16 bit data
#define MagDevice_Range_Microteslas      \
            (MagDevice_Maximum_Microteslas - MagDevice_Minimum_Microteslas)
#define MagDevice_Resolution_Microteslas \
            (MagDevice_Range_Microteslas / MagDevice_Precision)

// Magnetometer Sensor Unique ID
// {0746712D-DFB1-42A7-8AAE-6089D2423CDE}
DEFINE_GUID(GUID_MagDevice_UniqueID,
    0x746712d, 0xdfb1, 0x42a7, 0x8a, 0xae, 0x60, 0x89, 0xd2, 0x42, 0x3c, 0xde);

// Sensor data
typedef enum
{
    MAG_DATA_TIMESTAMP = 0,
    MAG_DATA_X,
    MAG_DATA_Y,
    MAG_DATA_Z,
    MAG_DATA_ACCURACY,
    MAG_DATA_COUNT
} MAG_DATA_INDEX;

// Sensor thresholds
typedef enum
{
    MAG_THRESHOLD_X = 0,
    MAG_THRESHOLD_Y,
    MAG_THRESHOLD_Z,
    MAG_THRESHOLD_COUNT
} MAG_THRESHOLD_INDEX;

//
// Sensor Enumeration Properties
//
typedef enum
{
    SENSOR_MAG_NAME = SENSOR_ENUMERATION_PROPERTIES_COUNT, // SENSOR_MAG_ENUMERATION_PROPERTIES_INDEX is adding SENSOR_MAG_NAME to the base SENSOR_ENUMERATION_PROPERTIES_INDEX enum.
                                                           // In order to keep the SENSOR_MAG_ENUMERATION_PROPERTIES_INDEX enum indexing coherent with the SENSOR_ENUMERATION_PROPERTIES_INDEX enum,
                                                           // set SENSOR_MAG_NAME to the index of the last value in the SENSOR_ENUMERATION_PROPERTIES_INDEX enum
    SENSOR_MAG_ENUMERATION_PROPERTIES_COUNT
} SENSOR_MAG_ENUMERATION_PROPERTIES_INDEX;



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
MagDevice::Initialize(
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

        Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_Timer);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! MAG WdfTimerCreate failed %!STATUS!", Status);
            goto Exit;
        }
    }

    //
    // Sensor Enumeration Properties
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_MAG_ENUMERATION_PROPERTIES_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_MAGNETOMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pEnumerationProperties);
        if (!NT_SUCCESS(Status) || m_pEnumerationProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! MAG WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
        m_pEnumerationProperties->Count = SENSOR_MAG_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_Magnetometer3D,
                                 &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"Manufacturer name",
                                  &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"MAG",
                                  &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Key = DEVPKEY_Sensor_ConnectionType;
        // The DEVPKEY_Sensor_ConnectionType values match the SensorConnectionType enumeration
        InitPropVariantFromUInt32(static_cast<ULONG>(SensorConnectionType::Integrated),
                                 &(m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_MagDevice_UniqueID,
                                 &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));

        m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Key = DEVPKEY_Sensor_IsPrimary;
        InitPropVariantFromBoolean(FALSE,
                                 &(m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Value));
        //
        // Alternate name used by SensorOpen
        //

        m_pEnumerationProperties->List[SENSOR_MAG_NAME].Key = DEVPKEY_Sensor_Name;
        InitPropVariantFromString(L"\\\\.\\Mag",
                                 &(m_pEnumerationProperties->List[SENSOR_MAG_NAME].Value));
    }

    //
    // Supported Data-Fields
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_PROPERTY_LIST_SIZE(MAG_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_MAGNETOMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pSupportedDataFields);
        if (!NT_SUCCESS(Status) || m_pSupportedDataFields == nullptr)
        {
            TraceError("COMBO %!FUNC! MAG WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
        m_pSupportedDataFields->Count = MAG_DATA_COUNT;

        m_pSupportedDataFields->List[MAG_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[MAG_DATA_X]         = PKEY_SensorData_MagneticFieldStrengthX_Microteslas;
        m_pSupportedDataFields->List[MAG_DATA_Y]         = PKEY_SensorData_MagneticFieldStrengthY_Microteslas;
        m_pSupportedDataFields->List[MAG_DATA_Z]         = PKEY_SensorData_MagneticFieldStrengthZ_Microteslas;
        m_pSupportedDataFields->List[MAG_DATA_ACCURACY]  = PKEY_SensorData_MagnetometerAccuracy;
    }

    //
    // Data
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(MAG_DATA_COUNT);
        FILETIME Time = {0};

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_MAGNETOMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pData);
        if (!NT_SUCCESS(Status) || m_pData == nullptr)
        {
            TraceError("COMBO %!FUNC! MAG WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
        m_pData->Count = MAG_DATA_COUNT;

        m_pData->List[MAG_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
        GetSystemTimePreciseAsFileTime(&Time);
        InitPropVariantFromFileTime(&Time, &(m_pData->List[MAG_DATA_TIMESTAMP].Value));

        m_pData->List[MAG_DATA_X].Key = PKEY_SensorData_MagneticFieldStrengthX_Microteslas;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[MAG_DATA_X].Value));

        m_pData->List[MAG_DATA_Y].Key = PKEY_SensorData_MagneticFieldStrengthY_Microteslas;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[MAG_DATA_Y].Value));

        m_pData->List[MAG_DATA_Z].Key = PKEY_SensorData_MagneticFieldStrengthZ_Microteslas;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[MAG_DATA_Z].Value));

        m_pData->List[MAG_DATA_ACCURACY].Key  = PKEY_SensorData_MagnetometerAccuracy;
        InitPropVariantFromUInt32(MagnetometerAccuracy_Unknown, &(m_pData->List[MAG_DATA_ACCURACY].Value));

        m_CachedData.Axis.X   = 0.0f;
        m_CachedData.Axis.Y   = 1.0f;
        m_CachedData.Axis.Z   = 0.0f;
        m_CachedData.Accuracy = MagnetometerAccuracy_High;

        m_LastSample.Axis.X   = 0.0f;
        m_LastSample.Axis.Y   = 0.0f;
        m_LastSample.Axis.Z   = 0.0f;
        m_LastSample.Accuracy = MagnetometerAccuracy_Unknown;
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
                                 SENSORV2_POOL_TAG_MAGNETOMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pProperties);
        if (!NT_SUCCESS(Status) || m_pProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! MAG WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
        m_pProperties->Count = SENSOR_COMMON_PROPERTY_COUNT;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(Mag_Initial_MinDataInterval_Ms,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Value));
        m_IntervalMs = Mag_Initial_MinDataInterval_Ms;
        m_MinimumIntervalMs = Mag_Initial_MinDataInterval_Ms;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_Magnetometer3D,
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
                                 SENSORV2_POOL_TAG_MAGNETOMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pDataFieldProperties);
        if (!NT_SUCCESS(Status) || m_pDataFieldProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! MAG WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
        m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

        m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat(MagDevice_Resolution_Microteslas,
                                 &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

        m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(MagDevice_Minimum_Microteslas,
                                 &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

        m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(MagDevice_Maximum_Microteslas,
                                 &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));
    }

    //
    // Set default threshold
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size =  SENSOR_COLLECTION_LIST_SIZE(MAG_THRESHOLD_COUNT);    //  Timestamp and shake do not have thresholds

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_MAGNETOMETER,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pThresholds);
        if (!NT_SUCCESS(Status) || m_pThresholds == nullptr)
        {
            TraceError("COMBO %!FUNC! MAG WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
        m_pThresholds->Count = MAG_THRESHOLD_COUNT;

        m_pThresholds->List[MAG_THRESHOLD_X].Key = PKEY_SensorData_MagneticFieldStrengthX_Microteslas;
        InitPropVariantFromFloat(Mag_Initial_Threshold_Microteslas,
                                    &(m_pThresholds->List[MAG_THRESHOLD_X].Value));

        m_pThresholds->List[MAG_THRESHOLD_Y].Key = PKEY_SensorData_MagneticFieldStrengthY_Microteslas;
        InitPropVariantFromFloat(Mag_Initial_Threshold_Microteslas,
                                    &(m_pThresholds->List[MAG_THRESHOLD_Y].Value));

        m_pThresholds->List[MAG_THRESHOLD_Z].Key = PKEY_SensorData_MagneticFieldStrengthZ_Microteslas;
        InitPropVariantFromFloat(Mag_Initial_Threshold_Microteslas,
                                    &(m_pThresholds->List[MAG_THRESHOLD_Z].Value));

        m_CachedThresholds.X = Mag_Initial_Threshold_Microteslas;
        m_CachedThresholds.Y = Mag_Initial_Threshold_Microteslas;
        m_CachedThresholds.Z = Mag_Initial_Threshold_Microteslas;

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
MagDevice::GetData(
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
            TraceError("COMBO %!FUNC! MAG GetPerformanceTime %!STATUS!", Status);
        }

        m_SampleCount = 0;

        DataReady = TRUE;
    }
    else
    {
        // Compare the change of data to threshold, and only push the data back to
        // clx if the change exceeds threshold. This is usually done in HW.
        if ( (abs(m_CachedData.Axis.X - m_LastSample.Axis.X) >= m_CachedThresholds.X) ||
             (abs(m_CachedData.Axis.Y - m_LastSample.Axis.Y) >= m_CachedThresholds.Y) ||
             (abs(m_CachedData.Axis.Z - m_LastSample.Axis.Z) >= m_CachedThresholds.Z))
        {
            DataReady = TRUE;
        }
    }

    if (DataReady != FALSE)
    {
        // update last sample
        m_LastSample.Axis.X   = m_CachedData.Axis.X;
        m_LastSample.Axis.Y   = m_CachedData.Axis.Y;
        m_LastSample.Axis.Z   = m_CachedData.Axis.Z;
        m_LastSample.Accuracy = m_CachedData.Accuracy;

        // push to clx
        InitPropVariantFromFloat (m_LastSample.Axis.X,   &(m_pData->List[MAG_DATA_X].Value));
        InitPropVariantFromFloat (m_LastSample.Axis.Y,   &(m_pData->List[MAG_DATA_Y].Value));
        InitPropVariantFromFloat (m_LastSample.Axis.Z,   &(m_pData->List[MAG_DATA_Z].Value));
        InitPropVariantFromUInt32(m_LastSample.Accuracy, &(m_pData->List[MAG_DATA_ACCURACY].Value));

        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[MAG_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("COMBO %!FUNC! MAG Data did NOT meet the threshold");
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
MagDevice::UpdateCachedThreshold(
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_MagneticFieldStrengthX_Microteslas,
                                    &m_CachedThresholds.X);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! MAG PropKeyFindKeyGetFloat for X failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_MagneticFieldStrengthY_Microteslas,
                                    &m_CachedThresholds.Y);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! MAG PropKeyFindKeyGetFloat for Y failed! %!STATUS!", Status);
        goto Exit;
    }

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_MagneticFieldStrengthZ_Microteslas,
                                    &m_CachedThresholds.Z);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! MAG PropKeyFindKeyGetFloat for Z failed! %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}