// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//     This module contains the type definitions for the client driver's device callback class.
//
// Environment:
//     Windows User-Mode Driver Framework (UMDF)

#pragma once

#include <windows.h>
#include <wdf.h>
#include <math.h>

#include "SensorsTrace.h"
#include <SensorsCx.h>
#include <SensorsUtils.h>

#define SENSOR_POOL_TAG_ACTIVITY 'itcA'
static const ULONG Act_Max_State_Count                              = 7;                     // Currently there are 7 states
static const ULONG Act_Default_MinDataInterval_Ms                   = 3000;                  // 3 seconds in milli seconds.
static const ULONG Act_Default_SubscribedStates                     = ActivityState_Max - 1; // all activities
static const BOOLEAN Act_Default_Streaming                          = FALSE;                 // no streaming
static const USHORT Act_Default_ConfidenceThreshold_Percentage      = 50;                    // 50% default confidence threshold
static const ULONG Act_Default_HistoryInterval_Ms                   = 60000;                 // 1 minute in milli seconds
static const ULONG Act_Default_MaxHistoryEntries                    = 60;                    // max of 60 history entries
static const ULONG Act_Default_Power_uW                             = 2000;                  // micro watts
static const ULONG Act_TimeoutForHistoryThread_Ms                   = 1000;                  // milli seconds
static const USHORT Act_ConfidenceForHistoryEntries_Percentage      = 70;                    // percentage
static const ULONG Act_First_Most_Probable_State                    = 0;

// Sensor Common Properties
typedef enum
{
    SENSOR_PROPERTY_STATE = 0,
    SENSOR_PROPERTY_MIN_INTERVAL,
    SENSOR_PROPERTY_MAX_DATAFIELDSIZE,
    SENSOR_PROPERTY_SENSOR_TYPE,
    SENSOR_PROPERTY_SENSOR_POWER,
    SENSOR_PROPERTY_SUPPORTEDACTIVITIES,
    SENSOR_PROPERTY_MAX_HISTORYSIZE,
    SENSOR_PROPERTY_HISTORY_INTERVAL,
    SENSOR_PROPERTY_MAX_HISTROYRECORDSIZE,
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

    // These enumeration properties overlap with a subset of the common properties,
    // and facilitate discovery without opening a sensor
    SENSOR_MIN_INTERVAL,
    SENSOR_POWER,
    SENSOR_SUPPORTEDACTIVITIES,
    SENSOR_MAX_HISTORYSIZE,
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
    ACTIVITY_DATA_TIMESTAMP = 0,
    ACTIVITY_DATA_CURRENT_STATE,
    ACTIVITY_DATA_CURRENT_CONFIDENCE,
    ACTIVITY_DATA_COUNT
} ACTIVITY_DATA_INDEX;

// Data Thresholds
typedef enum
{
    ACTIVITY_THRESHOLD_SUBSCRIBED_STATES = 0,
    ACTIVITY_THRESHOLD_STREAMING,
    ACTIVITY_THRESHOLD_CONFIDENCE,
    ACTIVITY_THRESHOLD_COUNT
} ACTIVITY_THRESHOLD_INDEX;

typedef struct _ActivitySample
{
    FILETIME Timestamp;
    ULONG Activity;
    UINT16 Confidence;
} ActivitySample, *PActivitySample;

// History buffer
typedef struct _HistoryCircularBuffer
{
    PActivitySample pData;
    ULONG FirstElemIndex;   // index of the oldest entry
    ULONG LastElemIndex;    // index of the latest entry
    ULONG NumOfElems;       // number of entries
    ULONG BufferLength;     // length of the buffer
} HistoryCircularBuffer, *PHistoryCircularBuffer;

typedef class _ActivityDevice
{
private:
    // Simulator
    WDFOBJECT                   m_SimulatorInstance;

    // WDF
    WDFDEVICE                   m_FxDevice;
    SENSOROBJECT                m_SensorInstance;
    WDFWAITLOCK                 m_Lock;
    WDFTIMER                    m_Timer;
    WDFWAITLOCK                 m_HistoryLock;
    WDFTIMER                    m_HistoryTimer;

    // Sensor operation
    BOOLEAN                     m_PoweredOn;
    BOOLEAN                     m_Started;
    ULONG                       m_Interval;
    BOOLEAN                     m_FirstSample;

    // History operation
    HistoryCircularBuffer       m_History;
    BOOLEAN                     m_HistoryStarted;
    ULONG                       m_HistoryIntervalInMs;
    ULONG                       m_HistorySizeInRecords;
    ULONG                       m_HistoryMarshalledRecordSize;
    ULONG                       m_HistoryPowerInuW;
    HANDLE                      m_hThread;
    HANDLE                      m_ExitEvent;
    BOOLEAN                     m_HistoryRetrievalStarted;
    PSENSOR_COLLECTION_LIST     m_ClientHistoryBuffer;
    ULONG                       m_ClientHistoryBufferSize;

    // Sensor Specific Properties
    PSENSOR_COLLECTION_LIST     m_pEnumerationProperties;
    PSENSOR_COLLECTION_LIST     m_pProperties;
    PSENSOR_PROPERTY_LIST       m_pSupportedDataFields;
    PSENSOR_COLLECTION_LIST     m_pDataFieldProperties;
    PSENSOR_COLLECTION_LIST     m_pThresholds;
    PSENSOR_COLLECTION_LIST     m_pLastSample;
    PSENSOR_COLLECTION_LIST     m_pFilteredSample;

public:
    // WDF callbacks
    static EVT_WDF_DRIVER_DEVICE_ADD                OnDeviceAdd;
    static EVT_WDF_DEVICE_PREPARE_HARDWARE          OnPrepareHardware;
    static EVT_WDF_DEVICE_RELEASE_HARDWARE          OnReleaseHardware;
    static EVT_WDF_DEVICE_D0_ENTRY                  OnD0Entry;
    static EVT_WDF_DEVICE_D0_EXIT                   OnD0Exit;
    static EVT_WDF_TIMER                            OnTimerExpire;
    static EVT_WDF_TIMER                            OnHistoryTimerExpire;

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

    // History retrieval
    static ULONG WINAPI HistoryRetrievalThread(_In_ LPVOID lpParam);

private:
    NTSTATUS                    Initialize(_In_ WDFDEVICE Device, _In_ SENSOROBJECT SensorObj);
    NTSTATUS                    InitializeSupportedDataFields();
    NTSTATUS                    InitializeSensorProperties();
    NTSTATUS                    InitializeEnumerationProperties();
    NTSTATUS                    InitializeDataFieldProperties();
    NTSTATUS                    GetData();
    _Requires_lock_held_(m_HistoryLock)
    NTSTATUS                    AddDataElementToHistoryBuffer(_In_ PActivitySample pData);
    _Requires_lock_held_(m_HistoryLock)
    NTSTATUS                    RemoveDataElementFromHistoryBuffer(_Out_ PActivitySample pData);
    _Requires_lock_held_(m_HistoryLock)
    NTSTATUS                    ClearHistoryBuffer();
} ActivityDevice, *PActivityDevice;

//
// Set up accessor function to retrieve device context
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ActivityDevice, GetActivityContextFromSensorInstance);
