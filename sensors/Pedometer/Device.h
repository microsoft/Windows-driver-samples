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

#define SENSOR_POOL_TAG_PEDOMETER 'odeP'

#define Pedometer_Default_MinDataInterval_Ms  (100) // milliseconds
#define Pedometer_Default_Threshold_StepCount (1) // threshold in step counts
#define Pedometer_Default_Power_Milliwatts    (2.0f) // milli watts
#define Pedometer_TimeoutForHistoryThread_Ms  (1000) // milli seconds
#define Pedometer_Default_HistoryInterval_Ms  (60000) // 1 minute in milli seconds
#define Pedometer_Default_MaxHistoryEntries   (60) // max of 60 history entries

// Sensor Common Properties
typedef enum
{
    SENSOR_PROPERTY_STATE = 0,
    SENSOR_PROPERTY_MIN_INTERVAL,
    SENSOR_PROPERTY_MAX_DATAFIELDSIZE,
    SENSOR_PROPERTY_SENSOR_TYPE,
    SENSOR_PROPERTY_SENSOR_POWER,
    SENSOR_PROPERTY_MAX_HISTORYSIZE,
    SENSOR_PROPERTY_HISTORY_INTERVAL,
    SENSOR_PROPERTY_MAX_HISTROYRECORDSIZE,
    SENSOR_PROPERTY_SUPPORTED_STEPTYPES,
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

    // These enumeration properties overlap with
    // a subset of the common properties, and
    // facilitate discovery without opening a sensor
    SENSOR_POWER,
    SENSOR_MAX_HISTORYSIZE,
    SENSOR_SUPPORTED_STEPTYPES,
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
    PEDOMETER_DATAFIELD_TIMESTAMP = 0,
    PEDOMETER_DATAFIELD_FIRST_AFTER_RESET,
    PEDOMETER_DATAFIELD_STEP_TYPE,
    PEDOMETER_DATAFIELD_STEP_COUNT,
    PEDOMETER_DATAFIELD_STEP_DURATION,
    PEDOMETER_DATAFIELD_COUNT
} PEDOMETER_DATAFIELD_INDEX;

// Supported Data
typedef enum
{
    PEDOMETER_DATA_TIMESTAMP = 0,
    PEDOMETER_DATA_FIRST_AFTER_RESET,
    PEDOMETER_DATA_UNKNOWN_STEP_TYPE,
    PEDOMETER_DATA_UNKNOWN_STEP_COUNT,
    PEDOMETER_DATA_UNKNOWN_STEP_DURATION,
    PEDOMETER_DATA_WALKING_STEP_TYPE,
    PEDOMETER_DATA_WALKING_STEP_COUNT,
    PEDOMETER_DATA_WALKING_STEP_DURATION,
    PEDOMETER_DATA_RUNNING_STEP_TYPE,
    PEDOMETER_DATA_RUNNING_STEP_COUNT,
    PEDOMETER_DATA_RUNNING_STEP_DURATION,
    PEDOMETER_DATA_COUNT
} PEDOMETER_DATA_INDEX;

typedef enum
{
    PEDOMETER_THRESHOLD_STEP_COUNT = 0,
    PEDOMETER_THRESHOLD_COUNT
} PEDOMETER_THRESHOLD_INDEX;

typedef struct PedometerSample
{
    FILETIME            Timestamp;
    BOOL                IsFirstAfterReset;
    ULONG               UnknownStepCount;
    INT64               UnknownStepDurationMs;
    ULONG               WalkingStepCount;
    INT64               WalkingStepDurationMs;
    ULONG               RunningStepCount;
    INT64               RunningStepDurationMs;
} PedometerSample, *PPedometerSample;

// History buffer
typedef struct
{
    PPedometerSample pData;
    ULONG FirstElemIndex; // index of the oldest entry
    ULONG LastElemIndex; // index of the latest entry
    ULONG NumOfElems; // number of entries
    ULONG BufferLength; // length of the buffer
} HistoryCircBuffer, *PHistoryCircBuffer;


typedef class PedometerDevice
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

    ULONG                       m_CachedThreshold;
    PedometerSample             m_LastSample;

    // History operation
    BOOLEAN                     m_HistorySupported;
    BOOLEAN                     m_HistoryRetrievalStarted;
    ULONG                       m_HistoryMarshalledRecordSize;
    HANDLE                      m_hThread;

    PSENSOR_COLLECTION_LIST     m_ClientHistoryBuffer;
    ULONG                       m_ClientHistoryBufferSize;

    // Sensor Specific Properties
    PSENSOR_COLLECTION_LIST     m_pEnumerationProperties;
    PSENSOR_COLLECTION_LIST     m_pProperties;
    PSENSOR_PROPERTY_LIST       m_pSupportedDataFields;
    PSENSOR_COLLECTION_LIST     m_pDataFieldProperties;
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
    static EVT_SENSOR_DRIVER_START_SENSOR_HISTORY       OnStartHistory;
    static EVT_SENSOR_DRIVER_STOP_SENSOR_HISTORY        OnStopHistory;
    static EVT_SENSOR_DRIVER_CLEAR_SENSOR_HISTORY       OnClearHistory;
    static EVT_SENSOR_DRIVER_START_HISTORY_RETRIEVAL    OnStartHistoryRetrieval;
    static EVT_SENSOR_DRIVER_CANCEL_HISTORY_RETRIEVAL   OnCancelHistoryRetrieval;
private:

    NTSTATUS                    Initialize(_In_ WDFDEVICE Device, _In_ SENSOROBJECT SensorObj);
    NTSTATUS                    GetData();
    VOID                        ResetPedometer();
    static ULONG WINAPI HistoryRetrievalThread(_In_ LPVOID lpParam);

} PedometerDevice, *PPedometerDevice;

// Set up accessor function to retrieve device context
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PedometerDevice, GetPedometerContextFromSensorInstance);

#define PedometerDevice_StepCount_Resolution           (1)
#define PedometerDevice_StepCount_Minimum              (0)
#define PedometerDevice_StepCount_Maximum              (0xFFFFFFFF)
