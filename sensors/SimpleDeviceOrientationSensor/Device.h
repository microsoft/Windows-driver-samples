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

#include <windows.devices.sensors.h>

#define SENSORV2_POOL_TAG_SDO 'sodS'

#define Sdo_Mininum_DataInterval     (20)  // 50Hz
#define Sdo_Default_DataInterval     (200) // 5Hz

//
// Sensor Common Properties
//
typedef enum
{
    SENSOR_PROPERTY_STATE = 0,
    SENSOR_PROPERTY_MIN_INTERVAL,
    SENSOR_PROPERTY_MAX_DATAFIELDSIZE,
    SENSOR_PROPERTY_SENSOR_TYPE,
    SENSOR_PROPERTIES_COUNT
} SENSOR_COMMON_PROPERTIES_INDEX;

//
// Sensor Enumeration Properties
//
typedef enum
{
    SENSOR_TYPE_GUID = 0,
    SENSOR_MANUFACTURER,
    SENSOR_MODEL,
    SENSOR_PERSISTENT_UNIQUEID,
    SENSOR_ISPRIMARY,
    SENSOR_ENUMERATION_PROPERTIES_COUNT
} SENSOR_ENUMERATION_PROPERTIES_INDEX;

//
// Supported Data Fields
//
typedef enum
{
    SDO_DATA_TIMESTAMP = 0,
    SDO_DATA_SIMPLEDEVICEORIENTATION,
    SDO_DATA_COUNT
} SDO_DATA_INDEX;


//
// Simple Device Orientation Driver Class
//

typedef class _SdoDevice
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

    // Sensor Specific Properties
    PSENSOR_PROPERTY_LIST       m_pSupportedDataFields;
    PSENSOR_COLLECTION_LIST     m_pEnumerationProperties;
    PSENSOR_COLLECTION_LIST     m_pProperties;
    PSENSOR_COLLECTION_LIST     m_pEmptyThreshold;

    //
    // SDO Operation ----------------------------------------------------
    //

    ULONG                                           m_Interval;
    BOOLEAN                                         m_FirstSample;
    ULONG                                           m_StartTime;
    ULONGLONG                                       m_SampleCount;

    PSENSOR_COLLECTION_LIST                         m_pData;                    // Sdo data that is going to push to clx
    ABI::Windows::Devices::Sensors::SimpleOrientation    m_LastSample;

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

} SdoDevice, *PSdoDevice;

//
// Set up accessor function to retrieve device context
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SdoDevice, GetSdoContextFromSensorInstance);
