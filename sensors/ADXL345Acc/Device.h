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
#include <reshub.h>
#include <strsafe.h>

#include <SensorsDef.h>
#include <SensorsCx.h>
#include <sensorsutils.h>
#include <SensorsDriversUtils.h>

#include "Adxl345.h"
#include "SensorsTrace.h"



#define SENSORV2_POOL_TAG_ACCELEROMETER '2ccA'


// Sensor Common Properties
typedef enum
{
    SENSOR_PROPERTY_STATE = 0,
    SENSOR_PROPERTY_MIN_DATA_INTERVAL,
    SENSOR_PROPERTY_MAX_DATA_FIELD_SIZE,
    SENSOR_PROPERTY_TYPE,
    SENSOR_PROPERTIES_COUNT
} SENSOR_PROPERTIES_INDEX;

// Sensor Enumeration Properties
typedef enum
{
    SENSOR_ENUMERATION_PROPERTY_TYPE = 0,
    SENSOR_ENUMERATION_PROPERTY_MANUFACTURER,
    SENSOR_ENUMERATION_PROPERTY_MODEL,
    SENSOR_ENUMERATION_PROPERTY_CONNECTION_TYPE,
    SENSOR_ENUMERATION_PROPERTY_PERSISTENT_UNIQUE_ID,
    SENSOR_ENUMERATION_PROPERTY_CATEGORY,
    SENSOR_ENUMERATION_PROPERTIES_COUNT
} SENSOR_ENUMERATION_PROPERTIES_INDEX;

// Data-field Properties
typedef enum
{
    SENSOR_DATA_ACCELERATION_X_G = 0,
    SENSOR_DATA_ACCELERATION_Y_G,
    SENSOR_DATA_ACCELERATION_Z_G,
    SENSOR_DATA_TIMESTAMP,
    SENSOR_DATA_COUNT
} SENSOR_DATA_INDEX;

typedef enum
{
    SENSOR_DATA_FIELD_PROPERTY_RESOLUTION = 0,
    SENSOR_DATA_FIELD_PROPERTY_RANGE_MIN,
    SENSOR_DATA_FIELD_PROPERTY_RANGE_MAX,
    SENSOR_DATA_FIELD_PROPERTIES_COUNT
} SENSOR_DATA_FIELD_PROPERTIES_INDEX;

typedef struct _REGISTER_SETTING
{
    BYTE Register;
    BYTE Value;
} REGISTER_SETTING, *PREGISTER_SETTING;


inline DATA_RATE _GetDataRateFromReportInterval(_In_ ULONG ReportInterval);

// Array of settings that describe the initial device configuration.
const REGISTER_SETTING g_ConfigurationSettings[] =
{
    // Standby mode
    { ADXL345_POWER_CTL, ADXL345_POWER_CTL_STANDBY },
    // +-16g, 13-bit resolution
    { ADXL345_DATA_FORMAT, 
      ADXL345_DATA_FORMAT_FULL_RES | ADXL345_DATA_FORMAT_JUSTIFY_RIGHT | ADXL345_DATA_FORMAT_RANGE_16G },
    // No FIFO
    { ADXL345_FIFO_CTL, ADXL345_FIFO_CTL_MODE_BYPASS },
    // Data rate set to default
    { ADXL345_BW_RATE, _GetDataRateFromReportInterval(DEFAULT_ACCELEROMETER_REPORT_INTERVAL).RateCode },
    // Activity threshold set to default change sensitivity
    { ADXL345_THRESH_ACT,
      static_cast<BYTE>(DEFAULT_ACCELEROMETER_CHANGE_SENSITIVITY / ACCELEROMETER_CHANGE_SENSITIVITY_RESOLUTION) },
    // Activity detection enabled, AC coupled
    { ADXL345_ACT_INACT_CTL, 
      ADXL345_ACT_INACT_CTL_ACT_ACDC | ADXL345_ACT_INACT_CTL_ACT_X | ADXL345_ACT_INACT_CTL_ACT_Y | ADXL345_ACT_INACT_CTL_ACT_Z },
    // Activity interrupt mapped to pin 1
    { ADXL345_INT_MAP, ADXL345_INT_ACTIVITY ^ ADXL345_INT_MASK},
};



typedef class _ADXL345AccDevice
{
private:
    // WDF
    WDFDEVICE                   m_Device;
    WDFIOTARGET                 m_I2CIoTarget;
    WDFWAITLOCK                 m_I2CWaitLock;
    WDFINTERRUPT                m_Interrupt;

    // Sensor Operation
    bool                        m_PoweredOn;
    bool                        m_Started;
    ULONG                       m_Interval;

    bool                        m_FirstSample;
    VEC3D                       m_CachedThresholds;
    VEC3D                       m_LastSample;

    SENSOROBJECT                m_SensorInstance;

    // Sensor Specific Properties
    PSENSOR_PROPERTY_LIST       m_pSupportedDataFields;
    PSENSOR_COLLECTION_LIST     m_pEnumerationProperties;
    PSENSOR_COLLECTION_LIST     m_pSensorProperties;
    PSENSOR_COLLECTION_LIST     m_pSensorData;
    PSENSOR_COLLECTION_LIST     m_pDataFieldProperties;
    PSENSOR_COLLECTION_LIST     m_pThresholds;

public:
    // WDF callbacks
    static EVT_WDF_DRIVER_DEVICE_ADD                OnDeviceAdd;
    static EVT_WDF_DEVICE_PREPARE_HARDWARE          OnPrepareHardware;
    static EVT_WDF_DEVICE_RELEASE_HARDWARE          OnReleaseHardware;
    static EVT_WDF_DEVICE_D0_ENTRY                  OnD0Entry;
    static EVT_WDF_DEVICE_D0_EXIT                   OnD0Exit;

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

    // Interrupt callbacks
    static EVT_WDF_INTERRUPT_ISR       OnInterruptIsr;
    static EVT_WDF_INTERRUPT_WORKITEM  OnInterruptWorkItem;

private:
    NTSTATUS                    GetData();

    // Helper function for OnPrepareHardware to initialize sensor to default properties
    NTSTATUS                    Initialize(_In_ WDFDEVICE Device, _In_ SENSOROBJECT SensorInstance);
    VOID                        DeInit();

    // Helper function for OnPrepareHardware to get resources from ACPI and configure the I/O target
    NTSTATUS                    ConfigureIoTarget(_In_ WDFCMRESLIST ResourceList,
                                                  _In_ WDFCMRESLIST ResourceListTranslated);

    // Helper function for OnD0Entry which sets up device to default configuration
    NTSTATUS                    PowerOn();
    NTSTATUS                    PowerOff();

} ADXL345AccDevice, *PADXL345AccDevice;

// Set up accessor function to retrieve device context
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ADXL345AccDevice, GetADXL345AccContextFromSensorInstance);