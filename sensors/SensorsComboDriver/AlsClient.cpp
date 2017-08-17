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

#include "AlsClient.tmh"

#define SENSORV2_POOL_TAG_AMBIENT_LIGHT           '2LmA'

#define Als_Initial_MinDataInterval_Ms            (10)          // 100Hz
#define Als_Initial_Lux_Threshold_Pct             (1.0f)        // Percent threshold: 100%
#define Als_Initial_Lux_Threshold_Abs             (0.0f)        // Absolute threshold: 0 lux
#define Als_Initial_Kelvin_Threshold_Abs          (100.0f)      // Absolute threshold: 100 Kelvins
#define Als_Initial_Chromaticity_X_Threshold_Abs  (0.01f)       // Absolute threshold: 0.01 of a CIE 1931 chromaticity x coordinate
#define Als_Initial_Chromaticity_Y_Threshold_Abs  (0.01f)       // Absolute threshold: 0.01 of a CIE 1931 chromaticity y coordinate

#define AlsDevice_Minimum_Lux                     (-4.0f)
#define AlsDevice_Maximum_Lux                     (4.0f)
#define AlsDevice_Precision                       (65536.0f)    // 65536 = 2^16, 16 bit data
#define AlsDevice_Range_Lux                       (AlsDevice_Maximum_Lux - AlsDevice_Minimum_Lux)
#define AlsDevice_Resolution_Lux                  (AlsDevice_Range_Lux / AlsDevice_Precision)

// Ambient Light Sensor Unique ID
// {2D2A4524-51E3-4E68-9B0F-5CAEDFB12C02}
DEFINE_GUID(GUID_AlsDevice_UniqueID,
    0x2d2a4524, 0x51e3, 0x4e68, 0x9b, 0xf, 0x5c, 0xae, 0xdf, 0xb1, 0x2c, 0x2);

// Sensor data
typedef enum
{
    ALS_DATA_TIMESTAMP = 0,
    ALS_DATA_LUX,
    ALS_DATA_KELVINS,
    ALS_DATA_CHROMATICITY_X,
    ALS_DATA_CHROMATICITY_Y,
    ALS_DATA_ISVALID,
    ALS_DATA_COUNT
} ALS_DATA_INDEX;

// Sensor thresholds
typedef enum
{
    ALS_THRESHOLD_LUX_PCT = 0,
    ALS_THRESHOLD_LUX_ABS,
    ALS_THRESHOLD_KELVINS_ABS,
    ALS_THRESHOLD_CHROMATICITY_X_ABS,
    ALS_THRESHOLD_CHROMATICITY_Y_ABS,
    ALS_THRESHOLD_COUNT
} ALS_THRESHOLD_INDEX;

//
// Sensor Enumeration Properties
//

typedef enum
{
    SENSOR_ALS_AUTOBRIGHTNESS_PREFERRED = SENSOR_ENUMERATION_PROPERTIES_COUNT, // SENSOR_ALS_ENUMERATION_PROPERTIES_INDEX is adding properties to the base SENSOR_ENUMERATION_PROPERTIES_COUNT enum.
    SENSOR_ALS_COLOR_CAPABLE,                                                  // In order to keep the SENSOR_ALS_ENUMERATION_PROPERTIES_INDEX enum indexing coherent with the SENSOR_ENUMERATION_PROPERTIES_INDEX enum,
                                                                               // set SENSOR_ALS_AUTOBRIGHTNESS_PREFERRED to the index of the last value in the SENSOR_ENUMERATION_PROPERTIES_INDEX enum
    SENSOR_ALS_ENUMERATION_PROPERTIES_COUNT
} SENSOR_ALS_ENUMERATION_PROPERTIES_INDEX;

typedef enum
{
    SENSOR_PROPERTY_ALS_RESPONSE_CURVE = SENSOR_COMMON_PROPERTY_COUNT, // SENSOR_ALS_PROPERTIES_INDEX is adding SENSOR_PROPERTY_ALS_RESPONSE_CURVE to the base SENSOR_COMMON_PROPERTIES_INDEX enum.
                                                                       // In order to keep the SENSOR_ALS_PROPERTIES_INDEX enum indexing coherent with the SENSOR_COMMON_PROPERTIES_INDEX enum,
                                                                       // set SENSOR_PROPERTY_ALS_RESPONSE_CURVE to the index of the last value in the SENSOR_COMMON_PROPERTIES_INDEX enum
    SENSOR_ALS_PROPERTY_COUNT
} SENSOR_ALS_PROPERTIES_INDEX;

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
AlsDevice::Initialize(
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
        TraceError("COMBO %!FUNC! ALS WdfWaitLockCreate failed %!STATUS!", Status);
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
            TraceError("COMBO %!FUNC! ALS WdfTimerCreate failed %!STATUS!", Status);
            goto Exit;
        }
    }

    //
    // Sensor Enumeration Properties
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ALS_ENUMERATION_PROPERTIES_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_AMBIENT_LIGHT,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pEnumerationProperties);
        if (!NT_SUCCESS(Status) || m_pEnumerationProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! ALS WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
        m_pEnumerationProperties->Count = SENSOR_ALS_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_AmbientLight,
                                 &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"Manufacturer name",
                                  &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"ALS",
                                  &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Key = DEVPKEY_Sensor_ConnectionType;
        // The DEVPKEY_Sensor_ConnectionType values match the SensorConnectionType enumeration
        InitPropVariantFromUInt32(static_cast<ULONG>(SensorConnectionType::Integrated),
                                 &(m_pEnumerationProperties->List[SENSOR_CONNECTION_TYPE].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_AlsDevice_UniqueID,
                                 &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));

        m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Key = DEVPKEY_Sensor_IsPrimary;
        InitPropVariantFromBoolean(TRUE,
                                 &(m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Value));

        m_pEnumerationProperties->List[SENSOR_ALS_AUTOBRIGHTNESS_PREFERRED].Key = DEVPKEY_LightSensor_AutoBrightnessPreferred;
        InitPropVariantFromBoolean(TRUE,
            &(m_pEnumerationProperties->List[SENSOR_ALS_AUTOBRIGHTNESS_PREFERRED].Value));

        m_pEnumerationProperties->List[SENSOR_ALS_COLOR_CAPABLE].Key = DEVPKEY_LightSensor_ColorCapable;
        InitPropVariantFromBoolean(TRUE,
            &(m_pEnumerationProperties->List[SENSOR_ALS_COLOR_CAPABLE].Value));
    }

    //
    // Supported Data-Fields
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_PROPERTY_LIST_SIZE(ALS_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_AMBIENT_LIGHT,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pSupportedDataFields);
        if (!NT_SUCCESS(Status) || m_pSupportedDataFields == nullptr)
        {
            TraceError("COMBO %!FUNC! ALS WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
        m_pSupportedDataFields->Count = ALS_DATA_COUNT;

        m_pSupportedDataFields->List[ALS_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[ALS_DATA_LUX] = PKEY_SensorData_LightLevel_Lux;
        m_pSupportedDataFields->List[ALS_DATA_KELVINS] = PKEY_SensorData_LightTemperature_Kelvins;
        m_pSupportedDataFields->List[ALS_DATA_CHROMATICITY_X] = PKEY_SensorData_LightChromaticityX;
        m_pSupportedDataFields->List[ALS_DATA_CHROMATICITY_Y] = PKEY_SensorData_LightChromaticityY;
        m_pSupportedDataFields->List[ALS_DATA_ISVALID] = PKEY_SensorData_IsValid;
    }

    //
    // Data
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(ALS_DATA_COUNT);
        FILETIME Time = {0};

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_AMBIENT_LIGHT,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pData);
        if (!NT_SUCCESS(Status) || m_pData == nullptr)
        {
            TraceError("COMBO %!FUNC! ALS WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
        m_pData->Count = ALS_DATA_COUNT;

        m_pData->List[ALS_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
        GetSystemTimePreciseAsFileTime(&Time);
        InitPropVariantFromFileTime(&Time, &(m_pData->List[ALS_DATA_TIMESTAMP].Value));

        m_pData->List[ALS_DATA_LUX].Key = PKEY_SensorData_LightLevel_Lux;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[ALS_DATA_LUX].Value));

        m_pData->List[ALS_DATA_KELVINS].Key = PKEY_SensorData_LightTemperature_Kelvins;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[ALS_DATA_KELVINS].Value));

        m_pData->List[ALS_DATA_CHROMATICITY_X].Key = PKEY_SensorData_LightChromaticityX;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[ALS_DATA_CHROMATICITY_X].Value));

        m_pData->List[ALS_DATA_CHROMATICITY_Y].Key = PKEY_SensorData_LightChromaticityY;
        InitPropVariantFromFloat(0.0f, &(m_pData->List[ALS_DATA_CHROMATICITY_Y].Value));

        m_pData->List[ALS_DATA_ISVALID].Key = PKEY_SensorData_IsValid;
        InitPropVariantFromBoolean(TRUE, &(m_pData->List[ALS_DATA_ISVALID].Value));

        m_CachedData = {
            1.0f, // Lux
            1.0f, // Kelvins
            0.5f, // Chromaticity X
            0.5f, // Chromaticity Y
            TRUE  // IsValid
        };

        m_LastSample = {
            0.0f, // Lux
            0.0f, // Kelvins
            0.0f, // Chromaticity X
            0.0f,  // Chromaticity Y
            TRUE   // IsValid
        };
    }

    //
    // Sensor Properties
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ALS_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_AMBIENT_LIGHT,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pProperties);
        if (!NT_SUCCESS(Status) || m_pProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! ALS WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
        m_pProperties->Count = SENSOR_ALS_PROPERTY_COUNT;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(Als_Initial_MinDataInterval_Ms,
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MIN_INTERVAL].Value));
        m_IntervalMs = Als_Initial_MinDataInterval_Ms;
        m_MinimumIntervalMs = Als_Initial_MinDataInterval_Ms;

        m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
                                  &(m_pProperties->List[SENSOR_COMMON_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_AmbientLight,
                                 &(m_pProperties->List[SENSOR_COMMON_PROPERTY_TYPE].Value));

        ULONG responseCurve[10] = {}; // Array to contain the response curve data.

        // ****************************************************************************************
        // The response curve consists of an array of byte pairs.
        // The first byte contains the percentage brightness offset to be applied to the display.
        // The second byte contains the corresponding ambient light value (in LUX).
        // ****************************************************************************************
        // (0, 10)
        responseCurve[0] = 0; responseCurve[1] = 10;
        // (10, 40)
        responseCurve[2] = 10; responseCurve[3] = 40;
        // (40, 100)
        responseCurve[4] = 40; responseCurve[5] = 100;
        // (68, 400)
        responseCurve[6] = 68; responseCurve[7] = 400;
        // (90, 1000)
        responseCurve[8] = 90; responseCurve[9] = 1000;

        m_pProperties->List[SENSOR_PROPERTY_ALS_RESPONSE_CURVE].Key = PKEY_LightSensor_ResponseCurve;
        InitPropVariantFromUInt32Vector(responseCurve,
            10,
            &(m_pProperties->List[SENSOR_PROPERTY_ALS_RESPONSE_CURVE].Value));
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
                                 SENSORV2_POOL_TAG_AMBIENT_LIGHT,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pDataFieldProperties);
        if (!NT_SUCCESS(Status) || m_pDataFieldProperties == nullptr)
        {
            TraceError("COMBO %!FUNC! ALS WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
        m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

        m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat(AlsDevice_Resolution_Lux,
                                 &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

        m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(AlsDevice_Minimum_Lux,
                                 &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

        m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(AlsDevice_Maximum_Lux,
                                 &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));
    }

    //
    // Set default threshold
    //
    {
        WDF_OBJECT_ATTRIBUTES MemoryAttributes;
        WDFMEMORY MemoryHandle = NULL;
        ULONG Size = SENSOR_COLLECTION_LIST_SIZE(ALS_THRESHOLD_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
                                 PagedPool,
                                 SENSORV2_POOL_TAG_AMBIENT_LIGHT,
                                 Size,
                                 &MemoryHandle,
                                 (PVOID*)&m_pThresholds);
        if (!NT_SUCCESS(Status) || m_pThresholds == nullptr)
        {
            TraceError("COMBO %!FUNC! ALS WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
        m_pThresholds->Count = ALS_THRESHOLD_COUNT;

        // Set lux threshold
        m_pThresholds->List[ALS_THRESHOLD_LUX_PCT].Key = PKEY_SensorData_LightLevel_Lux;
        InitPropVariantFromFloat(Als_Initial_Lux_Threshold_Pct,
                                    &(m_pThresholds->List[ALS_THRESHOLD_LUX_PCT].Value));
        m_CachedThresholds.LuxPct = Als_Initial_Lux_Threshold_Pct;

        m_pThresholds->List[ALS_THRESHOLD_LUX_ABS].Key = PKEY_SensorData_LightLevel_Lux_Threshold_AbsoluteDifference;
        InitPropVariantFromFloat(Als_Initial_Lux_Threshold_Abs,
                                    &(m_pThresholds->List[ALS_THRESHOLD_LUX_ABS].Value));
        m_CachedThresholds.LuxAbs = Als_Initial_Lux_Threshold_Abs;

        // Set kelvins threshold
        m_pThresholds->List[ALS_THRESHOLD_KELVINS_ABS].Key = PKEY_SensorData_LightTemperature_Kelvins;
        InitPropVariantFromFloat(Als_Initial_Kelvin_Threshold_Abs,
                                    &(m_pThresholds->List[ALS_THRESHOLD_KELVINS_ABS].Value));
        m_CachedThresholds.KelvinsAbs = Als_Initial_Kelvin_Threshold_Abs;

        // Set chromaticity x threshold
        m_pThresholds->List[ALS_THRESHOLD_CHROMATICITY_X_ABS].Key = PKEY_SensorData_LightChromaticityX;
        InitPropVariantFromFloat(Als_Initial_Chromaticity_X_Threshold_Abs,
                                    &(m_pThresholds->List[ALS_THRESHOLD_CHROMATICITY_X_ABS].Value));
        m_CachedThresholds.ChromaticityXAbs = Als_Initial_Chromaticity_X_Threshold_Abs;

        // Set chromaticity y threshold
        m_pThresholds->List[ALS_THRESHOLD_CHROMATICITY_Y_ABS].Key = PKEY_SensorData_LightChromaticityY;
        InitPropVariantFromFloat(Als_Initial_Chromaticity_Y_Threshold_Abs,
                                    &(m_pThresholds->List[ALS_THRESHOLD_CHROMATICITY_Y_ABS].Value));
        m_CachedThresholds.ChromaticityYAbs = Als_Initial_Chromaticity_Y_Threshold_Abs;

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
AlsDevice::GetData(
    )
{
    BOOLEAN DataReady = FALSE;
    FILETIME TimeStamp = {0};
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // new sample?
    if (m_FirstSample != FALSE)
    {
        Status = GetPerformanceTime(&m_StartTime);
        if (!NT_SUCCESS(Status))
        {
            m_StartTime = 0;
            TraceError("COMBO %!FUNC! ALS GetPerformanceTime %!STATUS!", Status);
        }

        m_SampleCount = 0;

        DataReady = TRUE;
    }
    else
    {
        // Compare the change of data to threshold, and only push the data back to
        // clx if the change exceeds threshold. This is usually done in HW.
        if (
            (
                // Lux thresholds needs to exceed absolute and percentage
                ((abs(m_CachedData.Lux - m_LastSample.Lux) >= (m_LastSample.Lux * m_CachedThresholds.LuxPct)) &&
                (abs(m_CachedData.Lux - m_LastSample.Lux) >= m_CachedThresholds.LuxAbs)) ||
                // If IsValid has changed, the sample is valid
                (m_CachedData.IsValid != m_LastSample.IsValid) ||
                // Kelvin temperature and Chromaticity thresholds
                (abs(m_CachedData.Kelvins - m_LastSample.Kelvins) >= m_CachedThresholds.KelvinsAbs) ||
                (abs(m_CachedData.ChromaticityX - m_LastSample.ChromaticityX) >= m_CachedThresholds.ChromaticityXAbs) ||
                (abs(m_CachedData.ChromaticityY - m_LastSample.ChromaticityY) >= m_CachedThresholds.ChromaticityYAbs)
            )
            &&
            (
                // In thresholded mode, don't send sample if the last sample sent was not IsValid and current sample is also not IsValid
                !((m_CachedThresholds.LuxAbs != 0.0f) && (m_CachedThresholds.LuxPct != 0.0f) &&
                  (m_CachedThresholds.KelvinsAbs != 0.0f) && (m_CachedThresholds.ChromaticityXAbs != 0.0f) &&
                  (m_CachedThresholds.ChromaticityYAbs != 0.0f) && !m_CachedData.IsValid && !m_LastSample.IsValid)
            )
           )
        {
            DataReady = TRUE;
        }
    }

    if (DataReady != FALSE)
    {
        // update last sample
        m_LastSample = m_CachedData;

        // push to clx
        InitPropVariantFromFloat(m_LastSample.Lux, &(m_pData->List[ALS_DATA_LUX].Value));

        InitPropVariantFromFloat(m_LastSample.Kelvins, &(m_pData->List[ALS_DATA_KELVINS].Value));

        InitPropVariantFromFloat(m_LastSample.ChromaticityX, &(m_pData->List[ALS_DATA_CHROMATICITY_X].Value));

        InitPropVariantFromFloat(m_LastSample.ChromaticityY, &(m_pData->List[ALS_DATA_CHROMATICITY_Y].Value));

        InitPropVariantFromBoolean(m_LastSample.IsValid, &(m_pData->List[ALS_DATA_ISVALID].Value));

        GetSystemTimePreciseAsFileTime(&TimeStamp);
        InitPropVariantFromFileTime(&TimeStamp, &(m_pData->List[ALS_DATA_TIMESTAMP].Value));

        SensorsCxSensorDataReady(m_SensorInstance, m_pData);
        m_FirstSample = FALSE;
    }
    else
    {
        Status = STATUS_DATA_NOT_ACCEPTED;
        TraceInformation("COMBO %!FUNC! ALS Data did NOT meet the threshold");
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
AlsDevice::UpdateCachedThreshold(
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                    &PKEY_SensorData_LightLevel_Lux,
                                    &m_CachedThresholds.LuxPct);
    if (!NT_SUCCESS(Status))
    {
        TraceError("COMBO %!FUNC! Failed to get lux pct data from cached threshold %!STATUS!", Status);
    }

    if (NT_SUCCESS(Status))
    {
        Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                        &PKEY_SensorData_LightLevel_Lux_Threshold_AbsoluteDifference,
                                        &m_CachedThresholds.LuxAbs);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! Failed to get lux abs data from cached threshold %!STATUS!", Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                        &PKEY_SensorData_LightTemperature_Kelvins,
                                        &m_CachedThresholds.KelvinsAbs);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! Failed to get kelvin data from cached threshold %!STATUS!", Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                        &PKEY_SensorData_LightChromaticityX,
                                        &m_CachedThresholds.ChromaticityXAbs);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! Failed to get chromaticity x data from cached threshold %!STATUS!", Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = PropKeyFindKeyGetFloat(m_pThresholds,
                                        &PKEY_SensorData_LightChromaticityY,
                                        &m_CachedThresholds.ChromaticityYAbs);
        if (!NT_SUCCESS(Status))
        {
            TraceError("COMBO %!FUNC! Failed to get chromaticity y data from cached threshold %!STATUS!", Status);
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}