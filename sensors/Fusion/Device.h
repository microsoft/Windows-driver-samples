//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//
//    This module contains the type definitions for the client
//    driver's device callback class.
//
//Environment:
//
//    Windows User-Mode Driver Framework (UMDF)

#pragma once

#include <windows.h>
#include <wdf.h>
#include <math.h>

#include "SensorsTrace.h"
#include <SensorsCx.h>
#include <SensorsUtils.h>
#include <Math3DHelper.h>
#include <SensorsDef.h>

#define SENSOR_POOL_TAG_FUSIONSENSOR 'esuF'

#define FusionSensor_Default_MinDataInterval_Ms  (16)  // 60Hz
#define FusionSensor_Default_DataInterval     (100) // 10Hz
#define FusionSensor_Quarternion_Maximum      (1.0f)
#define FusionSensor_Quarternion_Minimum      (-1.0f)
#define FusionSensor_Quarternion_Resolution   ((FusionSensor_Quarternion_Maximum-FusionSensor_Quarternion_Minimum)/65536)


// TODO: Customize the following 5 lines based on the hardware being used
#define GyrFakeDevice_Minimum_DegreesPerSecond    (-2000.0f)
#define GyrFakeDevice_Maximum_DegreesPerSecond    (2000.0f)
#define GyrFakeDevice_Precision                   (65536.0f)    // 65536 = 2^16, 16 bit data
#define GyrFakeDevice_Range_DegreesPerSecond      (GyrFakeDevice_Maximum_DegreesPerSecond - GyrFakeDevice_Minimum_DegreesPerSecond)
#define GyrFakeDevice_Resolution_DegreesPerSecond (GyrFakeDevice_Range_DegreesPerSecond / GyrFakeDevice_Precision)

// TODO: Customize the following 3 lines based on the hardware being used
#define AccFakeDevice_Axis_Resolution           (4.0f / 65536.0f) // in delta g
#define AccFakeDevice_Axis_Minimum              (-2.0f)           // in g
#define AccFakeDevice_Axis_Maximum              (2.0f)            // in g

// Sensor Common Properties
typedef enum
{
    SENSOR_PROPERTY_STATE = 0,
    SENSOR_PROPERTY_MIN_INTERVAL,
    SENSOR_PROPERTY_MAX_DATAFIELDSIZE,
    SENSOR_PROPERTY_SENSOR_TYPE,
    SENSOR_PROPERTY_USE_GYRO,
    SENSOR_PROPERTIES_COUNT
} SENSOR_COMMON_PROPERTIES_INDEX;

// Sensor Enumeration Properties
typedef enum
{
    SENSOR_TYPE_GUID = 0,
    SENSOR_MANUFACTURER,
    SENSOR_MODEL,
    SENSOR_PERSISTENT_UNIQUEID,
    SENSOR_ISPRIMARY,
    SENSOR_ENUMERATION_PROPERTIES_COUNT
} SENSOR_ENUMERATION_PROPERTIES_INDEX;

// Accelerometer related data-field Properties
typedef enum
{
    SENSOR_ACC_RESOLUTION = 0,
    SENSOR_ACC_MIN_RANGE,
    SENSOR_ACC_MAX_RANGE,
    SENSOR_ACC_DATA_FIELD_PROPERTY_COUNT
} SENSOR_ACC_DATA_FIELD_PROPERTY_INDEX;

// Gyroscope related data-field Properties
typedef enum
{
    SENSOR_GYR_RESOLUTION = 0,
    SENSOR_GYR_MIN_RANGE,
    SENSOR_GYR_MAX_RANGE,
    SENSOR_GYR_DATA_FIELD_PROPERTY_COUNT
} SENSOR_GYR_DATA_FIELD_PROPERTY_INDEX;

// Supported Data
typedef enum
{
    FUSIONSENSOR_DATA_TIMESTAMP = 0,
    FUSIONSENSOR_DATA_QUATERNION_W,
    FUSIONSENSOR_DATA_QUATERNION_X,
    FUSIONSENSOR_DATA_QUATERNION_Y,
    FUSIONSENSOR_DATA_QUATERNION_Z,
    FUSIONSENSOR_DATA_ACCURACY,
    FUSIONSENSOR_DATA_DECLINATION_ANGLE,
    FUSIONSENSOR_DATA_COUNT
} FUSIONSENSOR_DATA_INDEX;

typedef enum
{
    FUSIONSENSOR_THRESHOLD_ROTATION_ANGLE = 0,
    FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_X,
    FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_Y,
    FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_Z,
    FUSIONSENSOR_THRESHOLD_ROTATION_RATE_X,
    FUSIONSENSOR_THRESHOLD_ROTATION_RATE_Y,
    FUSIONSENSOR_THRESHOLD_ROTATION_RATE_Z,
    FUSIONSENSOR_THRESHOLD_COUNT
} FUSIONSENSOR_THRESHOLD_INDEX;

typedef struct FusionSensorSample
{
    FILETIME    Timestamp;
    QUATERNION  Quaternion;
    ULONG       Accuracy;
    FLOAT       DeclinationAngle;
} FusionSensorSample, *PFusionSensorSample;

//
// Fusion Threshold
//
typedef struct _FusThreshold
{
    FLOAT       RotationAngle;
    VEC3D       LinearAcceleration;
    VEC3D       RotationRate;
} FusThreshold, *PFusThreshold;

typedef class FusionSensorDevice
{
private:
    // Simulator
    WDFOBJECT                   m_SimulatorInstance;

    // WDF
    WDFDEVICE                   m_FxDevice;
    SENSOROBJECT                m_SensorInstance;
    WDFWAITLOCK                 m_Lock;
    WDFTIMER                    m_Timer;

    // Sensor operation
    BOOLEAN                     m_PoweredOn;
    BOOLEAN                     m_Started;
    ULONG                       m_Interval;

    BOOLEAN                     m_FirstSample;
    ULONG                       m_StartTime;
    ULONGLONG                   m_SampleCount;

    FusThreshold                m_CachedThresholds;

    FusionSensorSample          m_LastSample;

    // Sensor Specific Properties
    PSENSOR_COLLECTION_LIST     m_pEnumerationProperties;
    PSENSOR_COLLECTION_LIST     m_pProperties;
    PSENSOR_PROPERTY_LIST       m_pSupportedDataFields;
    PSENSOR_COLLECTION_LIST     m_pAccDataFieldProperties; // Accelerometer related datafield properties
    PSENSOR_COLLECTION_LIST     m_pGyrDataFieldProperties; // Gyroscope related datafield properties
    PSENSOR_COLLECTION_LIST     m_pThresholds;
    PSENSOR_COLLECTION_LIST     m_pData;

public:
    // WDF callbacks
    static EVT_WDF_DRIVER_DEVICE_ADD                OnDeviceAdd;
    static EVT_WDF_DEVICE_PREPARE_HARDWARE          OnPrepareHardware;
    static EVT_WDF_DEVICE_RELEASE_HARDWARE          OnReleaseHardware;
    static EVT_WDF_DEVICE_D0_ENTRY                  OnD0Entry;
    static EVT_WDF_DEVICE_D0_EXIT                   OnD0Exit;
    static EVT_WDF_TIMER                            OnTimerExpire;

    // CLX callbacks
    static EVT_SENSOR_DRIVER_START_SENSOR               OnStart;
    static EVT_SENSOR_DRIVER_STOP_SENSOR                OnStop;
    static EVT_SENSOR_DRIVER_GET_SUPPORTED_DATA_FIELDS  OnGetSupportedDataFields;
    static EVT_SENSOR_DRIVER_GET_PROPERTIES             OnGetProperties;
    static EVT_SENSOR_DRIVER_GET_DATA_FIELD_PROPERTIES  OnGetDataFieldProperties;
    static EVT_SENSOR_DRIVER_GET_DATA_INTERVAL          OnGetDataInterval;
    static EVT_SENSOR_DRIVER_SET_DATA_INTERVAL          OnSetDataInterval;
    static EVT_SENSOR_DRIVER_GET_DATA_THRESHOLDS        OnGetDataThresholds;
    static EVT_SENSOR_DRIVER_SET_DATA_THRESHOLDS        OnSetDataThresholds;
    static EVT_SENSOR_DRIVER_DEVICE_IO_CONTROL          OnIoControl;

private:

    NTSTATUS                    Initialize(_In_ WDFDEVICE Device, _In_ SENSOROBJECT SensorObj);
    NTSTATUS                    GetData();

} FusionSensorDevice, *PFusionSensorDevice;

// Set up accessor function to retrieve device context
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FusionSensorDevice, GetFusionSensorContextFromSensorInstance);

#define FusionSensorDevice_StepCount_Resolution           (1)
#define FusionSensorDevice_StepCount_Minimum              (0)
#define FusionSensorDevice_StepCount_Maximum              (0xFFFFFFFF)
