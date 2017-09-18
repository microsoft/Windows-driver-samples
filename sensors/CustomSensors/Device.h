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

#include "HardwareSimulator.h"

#define SENSORV2_POOL_TAG_CUSTOM_SENSOR 'mtsC'

#define Cstm_Default_MinDataInterval_Ms (200)  // 200 milliseconds interval is sufficient for this CO2 sensor


// Sensor Common Properties
typedef enum
{
    SENSOR_PROPERTY_STATE = 0,
    SENSOR_PROPERTY_MIN_INTERVAL,
    SENSOR_PROPERTY_MAX_DATAFIELDSIZE,
    SENSOR_PROPERTY_SENSOR_TYPE,
    SENSOR_COMMON_PROPERTIES_COUNT
} SENSOR_COMMON_PROPERTIES_INDEX;

// Sensor Enumeration Properties
typedef enum
{
    SENSOR_TYPE_GUID = 0,
    SENSOR_MANUFACTURER,
    SENSOR_MODEL,
    SENSOR_PERSISTENT_UNIQUEID,
    SENSOR_CATEGORY,
    SENSOR_ISPRIMARY,
    SENSOR_VENDOR_DEFINED_TYPE,
    SENSOR_ENUMERATION_PROPERTIES_COUNT
} SENSOR_ENUMERATION_PROPERTIES_INDEX;

// Data-field Properties
typedef enum
{
    SENSOR_RESOLUTION = 0,
    SENSOR_MIN_RANGE,
    SENSOR_MAX_RANGE,
    SENSOR_DATA_FIELD_PROPERTY_COUNT
} SENSOR_DATA_FIELD_PROPERTY_INDEX;

// Supported Data Fields
typedef enum
{
    CSTM_DATA_CO2_LEVEL_PERCENT = 0,
    CSTM_DATA_TIMESTAMP,
    CSTM_DATA_COUNT
} CSTM_DATA_INDEX;

typedef struct _CustomSensorSample
{
    FLOAT AtmosphericCO2Level;
} CustomSensorSample, *PCustomSensorSample;

typedef class _CustomSensorDevice
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

    CustomSensorSample          m_LastSample;

    // Sensor Specific Properties
    PSENSOR_COLLECTION_LIST     m_pEnumerationProperties;
    PSENSOR_COLLECTION_LIST     m_pProperties;
    PSENSOR_PROPERTY_LIST       m_pSupportedDataFields;
    PSENSOR_COLLECTION_LIST     m_pDataFieldProperties;
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

} CustomSensorDevice, *PCustomSensorDevice;

// Set up accessor function to retrieve device context
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CustomSensorDevice, GetCustomSensorContextFromSensorInstance);

#define CustomSensorDevice_Resolution           (0.1f)     // CO2 concentration resolution in parts per million (ppmv).
#define CustomSensorDevice_Minimum_CO2Level     (0.0f)     // Minimum CO2 concentration in parts per million (ppmv).
#define CustomSensorDevice_Maximum_CO2Level     (10000.0f) // Maximum CO2 concentration in parts per million (ppmv).
