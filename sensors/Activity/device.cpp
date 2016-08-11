// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//     This module contains the type definitions for the client driver's device callback class.
//
// Environment:
//     Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "HardwareSimulator.h"
#include "Device.tmh"

// Activity Unique ID {8b3dd227-cc0e-465f-a36e-811c94a1b240}
DEFINE_GUID(GUID_ActivityDevice_UniqueID, 0x8b3dd227, 0xcc0e, 0x465f, 0xa3, 0x6e, 0x81, 0x1c, 0x94, 0xa1, 0xb2, 0x40);

static const float ActivityFakeDevice_Confidence_Resolution = 1.0f;
static const float ActivityFakeDevice_Confidence_Minimum = 0.0f;
static const float ActivityFakeDevice_Confidence_Maximum = 100.0f;

// This routine initializes the sensor's enumeration properties
NTSTATUS ActivityDevice::InitializeEnumerationProperties()
{
    WDF_OBJECT_ATTRIBUTES memoryAttributes = {};
    WDFMEMORY memoryHandle = NULL;
    const ULONG size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ENUMERATION_PROPERTIES_COUNT);
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
    memoryAttributes.ParentObject = m_SensorInstance;
    status = WdfMemoryCreate(&memoryAttributes,
        PagedPool,
        SENSOR_POOL_TAG_ACTIVITY,
        size,
        &memoryHandle,
        reinterpret_cast<PVOID*>(&m_pEnumerationProperties));
    if (!NT_SUCCESS(status) || nullptr == m_pEnumerationProperties)
    {
        TraceError("ACT %!FUNC! WdfMemoryCreate failed %!STATUS!", status);
    }
    else
    {
        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, size);
        m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_ActivityDetection,
            &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"Microsoft",
            &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"Fake ACTIVITY V2",
            &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_ActivityDevice_UniqueID,
            &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));

        m_pEnumerationProperties->List[SENSOR_CATEGORY].Key = DEVPKEY_Sensor_Category;
        InitPropVariantFromCLSID(GUID_SensorCategory_Motion,
            &(m_pEnumerationProperties->List[SENSOR_CATEGORY].Value));

        m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Key = DEVPKEY_Sensor_IsPrimary;
        InitPropVariantFromBoolean(FALSE,
            &(m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Value));

        m_pEnumerationProperties->List[SENSOR_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(Act_Default_MinDataInterval_Ms,
            &(m_pEnumerationProperties->List[SENSOR_MIN_INTERVAL].Value));

        m_pEnumerationProperties->List[SENSOR_POWER].Key = PKEY_Sensor_Power_Milliwatts;
        InitPropVariantFromFloat((m_HistoryPowerInuW / 1000.0f),
            &(m_pEnumerationProperties->List[SENSOR_POWER].Value));

        m_pEnumerationProperties->List[SENSOR_SUPPORTEDACTIVITIES].Key = PKEY_SensorData_SupportedActivityStates;
        InitPropVariantFromUInt32(Act_Default_SubscribedStates,
            &(m_pEnumerationProperties->List[SENSOR_SUPPORTEDACTIVITIES].Value));

        m_pEnumerationProperties->List[SENSOR_MAX_HISTORYSIZE].Key = PKEY_SensorHistory_MaxSize_Bytes;
        InitPropVariantFromUInt32(SENSOR_COLLECTION_LIST_HEADER_SIZE + ((m_HistoryMarshalledRecordSize - SENSOR_COLLECTION_LIST_HEADER_SIZE) * m_HistorySizeInRecords),
            &(m_pEnumerationProperties->List[SENSOR_MAX_HISTORYSIZE].Value));   // History only logs one most probable state
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine initializes the sensor's properties
NTSTATUS ActivityDevice::InitializeSensorProperties()
{
    WDF_OBJECT_ATTRIBUTES memoryAttributes = {};
    WDFMEMORY memoryHandle = NULL;
    const ULONG size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_COMMON_PROPERTIES_COUNT);
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
    memoryAttributes.ParentObject = m_SensorInstance;
    status = WdfMemoryCreate(&memoryAttributes,
        PagedPool,
        SENSOR_POOL_TAG_ACTIVITY,
        size,
        &memoryHandle,
        reinterpret_cast<PVOID*>(&m_pProperties));
    if (!NT_SUCCESS(status) || nullptr == m_pProperties)
    {
        TraceError("ACT %!FUNC! WdfMemoryCreate failed %!STATUS!", status);
    }
    else
    {
        SENSOR_COLLECTION_LIST_INIT(m_pProperties, size);
        m_pProperties->Count = SENSOR_COMMON_PROPERTIES_COUNT;

        m_pProperties->List[SENSOR_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
            &(m_pProperties->List[SENSOR_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(Act_Default_MinDataInterval_Ms,
            &(m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Value));

        m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pLastSample),
            &(m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_ActivityDetection,
            &(m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Value)); 

        m_pProperties->List[SENSOR_PROPERTY_SENSOR_POWER].Key = PKEY_Sensor_Power_Milliwatts;
        InitPropVariantFromFloat((m_HistoryPowerInuW / 1000.0f),
            &(m_pProperties->List[SENSOR_PROPERTY_SENSOR_POWER].Value));

        m_pProperties->List[SENSOR_PROPERTY_SUPPORTEDACTIVITIES].Key = PKEY_SensorData_SupportedActivityStates;
        InitPropVariantFromUInt32(Act_Default_SubscribedStates,
            &(m_pProperties->List[SENSOR_PROPERTY_SUPPORTEDACTIVITIES].Value));

        m_pProperties->List[SENSOR_PROPERTY_MAX_HISTORYSIZE].Key = PKEY_SensorHistory_MaxSize_Bytes;
        InitPropVariantFromUInt32(SENSOR_COLLECTION_LIST_HEADER_SIZE + ((m_HistoryMarshalledRecordSize - SENSOR_COLLECTION_LIST_HEADER_SIZE) * m_HistorySizeInRecords),
            &(m_pProperties->List[SENSOR_PROPERTY_MAX_HISTORYSIZE].Value)); // History only logs one most probable state

        m_pProperties->List[SENSOR_PROPERTY_HISTORY_INTERVAL].Key = PKEY_SensorHistory_Interval_Ms;
        InitPropVariantFromUInt32(m_HistoryIntervalInMs,
            &(m_pProperties->List[SENSOR_PROPERTY_HISTORY_INTERVAL].Value));

        m_pProperties->List[SENSOR_PROPERTY_MAX_HISTROYRECORDSIZE].Key = PKEY_SensorHistory_MaximumRecordSize_Bytes;
        InitPropVariantFromUInt32(m_HistoryMarshalledRecordSize,
            &(m_pProperties->List[SENSOR_PROPERTY_MAX_HISTROYRECORDSIZE].Value));
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine initializes the data field properties
NTSTATUS ActivityDevice::InitializeDataFieldProperties()
{
    WDF_OBJECT_ATTRIBUTES memoryAttributes = {};
    WDFMEMORY memoryHandle = NULL;
    const ULONG size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_DATA_FIELD_PROPERTY_COUNT);
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
    memoryAttributes.ParentObject = m_SensorInstance;
    status = WdfMemoryCreate(&memoryAttributes,
        PagedPool,
        SENSOR_POOL_TAG_ACTIVITY,
        size,
        &memoryHandle,
        reinterpret_cast<PVOID*>(&m_pDataFieldProperties));
    if (!NT_SUCCESS(status) || nullptr == m_pDataFieldProperties)
    {
        TraceError("ACT %!FUNC! WdfMemoryCreate failed %!STATUS!", status);
    }
    else
    {
        SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, size);
        m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

        m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat(ActivityFakeDevice_Confidence_Resolution,
            &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

        m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(ActivityFakeDevice_Confidence_Minimum,
            &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

        m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(ActivityFakeDevice_Confidence_Maximum,
            &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine initializes the supported data fields
NTSTATUS ActivityDevice::InitializeSupportedDataFields()
{
    WDF_OBJECT_ATTRIBUTES memoryAttributes = {};
    WDFMEMORY memoryHandle = NULL;
    const ULONG size = SENSOR_PROPERTY_LIST_SIZE(ACTIVITY_DATA_COUNT);
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
    memoryAttributes.ParentObject = m_SensorInstance;
    status = WdfMemoryCreate(&memoryAttributes,
        PagedPool,
        SENSOR_POOL_TAG_ACTIVITY,
        size,
        &memoryHandle,
        reinterpret_cast<PVOID*>(&m_pSupportedDataFields));
    if (!NT_SUCCESS(status) || nullptr == m_pSupportedDataFields)
    {
        TraceError("ACT %!FUNC! WdfMemoryCreate failed %!STATUS!", status);
    }
    else
    {
        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, size);
        m_pSupportedDataFields->Count = ACTIVITY_DATA_COUNT;

        m_pSupportedDataFields->List[ACTIVITY_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[ACTIVITY_DATA_CURRENT_STATE] = PKEY_SensorData_CurrentActivityState;
        m_pSupportedDataFields->List[ACTIVITY_DATA_CURRENT_CONFIDENCE] = PKEY_SensorData_CurrentActivityStateConfidence_Percentage;
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine initializes the sensor to its default properties
NTSTATUS ActivityDevice::Initialize(
    _In_ WDFDEVICE device,              // WDFDEVICE object
    _In_ SENSOROBJECT sensorInstance)   // SENSOROBJECT for each sensor instance
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Initial configuration
    m_FxDevice = device;
    m_SensorInstance = sensorInstance;
    m_Interval = Act_Default_MinDataInterval_Ms;
    m_FirstSample = TRUE;
    m_Started = FALSE;
    m_HistorySizeInRecords = Act_Default_MaxHistoryEntries;
    m_HistoryPowerInuW = Act_Default_Power_uW;
    m_HistoryIntervalInMs = Act_Default_HistoryInterval_Ms;
    m_HistoryStarted = FALSE;
    m_HistoryRetrievalStarted = FALSE;
    m_History.FirstElemIndex = 0;
    m_History.LastElemIndex = 0;
    m_History.NumOfElems = 0;
    m_History.BufferLength = m_HistorySizeInRecords;
    m_hThread = NULL;

    // Initialize the activity simulator
    status = HardwareSimulator::Initialize(device, &m_SimulatorInstance);
    if (!NT_SUCCESS(status))
    {
        TraceError("ACT %!FUNC! HardwareSimulator::Initialize failed %!STATUS!", status);
        status = STATUS_SUCCESS;    // Failed to set up simulator should not fail the driver
    }

    // Create Lock
    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
    if (!NT_SUCCESS(status))
    {
        TraceError("ACT %!FUNC! WdfWaitLockCreate failed %!STATUS!", status);
    }

    // Create history lock
    if (NT_SUCCESS(status))
    {
        status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_HistoryLock);
        if (!NT_SUCCESS(status))
        {
            TraceError("ACT %!FUNC! WdfWaitLockCreate failed %!STATUS!", status);
        }
    }

    // Create timer object for polling sensor samples
    if (NT_SUCCESS(status))
    {
        WDF_OBJECT_ATTRIBUTES timerAttributes = {};
        WDF_TIMER_CONFIG timerConfig = {};

        WDF_TIMER_CONFIG_INIT(&timerConfig, ActivityDevice::OnTimerExpire);
        WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
        timerAttributes.ParentObject = sensorInstance;
        timerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

        status = WdfTimerCreate(&timerConfig, &timerAttributes, &m_Timer);
        if (!NT_SUCCESS(status))
        {
            TraceError("ACT %!FUNC! WdfTimerCreate failed %!STATUS!", status);
        }
    }

    // Create timer object for keeping history
    if (NT_SUCCESS(status))
    {
        WDF_OBJECT_ATTRIBUTES timerAttributes = {};
        WDF_TIMER_CONFIG timerConfig = {};

        WDF_TIMER_CONFIG_INIT(&timerConfig, ActivityDevice::OnHistoryTimerExpire);
        WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
        timerAttributes.ParentObject = sensorInstance;
        timerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

        status = WdfTimerCreate(&timerConfig, &timerAttributes, &m_HistoryTimer);
        if (!NT_SUCCESS(status))
        {
            TraceError("ACT %!FUNC! WdfTimerCreate for history failed %!STATUS!", status);
        }
    }

    // Last available data
    if (NT_SUCCESS(status))
    {
        // Allocate a buffer for max state count. 7 States and 1 timestamp, each 
        // state has activity  and confidence. The actual size will be adjusted when 
        // data is ready to be pushed to clx.
        const ULONG size = SENSOR_COLLECTION_LIST_SIZE(Act_Max_State_Count * 2 + 1);
        WDF_OBJECT_ATTRIBUTES memoryAttributes = {};
        WDFMEMORY memoryHandle = NULL;

        WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
        memoryAttributes.ParentObject = sensorInstance;
        status = WdfMemoryCreate(&memoryAttributes, 
            PagedPool, 
            SENSOR_POOL_TAG_ACTIVITY, 
            size, 
            &memoryHandle, 
            reinterpret_cast<PVOID*>(&m_pLastSample));
        if (!NT_SUCCESS(status) || nullptr == m_pLastSample)
        {
            TraceError("ACT %!FUNC! WdfMemoryCreate failed %!STATUS!", status);
        }
        else
        {
            FILETIME time = {};
            SENSOR_COLLECTION_LIST_INIT(m_pLastSample, size);
            m_pLastSample->Count = Act_Max_State_Count * 2 + 1;
            m_pLastSample->List[ACTIVITY_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
            InitPropVariantFromFileTime(&time, &(m_pLastSample->List[ACTIVITY_DATA_TIMESTAMP].Value));
            for (ULONG Count = 1; Count < Act_Max_State_Count * 2 + 1; Count += 2)
            {
                m_pLastSample->List[Count].Key = PKEY_SensorData_CurrentActivityState;
                InitPropVariantFromUInt32(ActivityState_Stationary, &(m_pLastSample->List[Count].Value));
                m_pLastSample->List[Count + 1].Key = PKEY_SensorData_CurrentActivityStateConfidence_Percentage;
                InitPropVariantFromUInt16(100, &(m_pLastSample->List[Count + 1].Value));
            }
        }
    }

    // Filtered data
    if (NT_SUCCESS(status) && nullptr != m_pLastSample && 0 != m_pLastSample->AllocatedSizeInBytes)
    {
        WDF_OBJECT_ATTRIBUTES memoryAttributes = {};
        WDFMEMORY memoryHandle = NULL;

        WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
        memoryAttributes.ParentObject = sensorInstance;
        status = WdfMemoryCreate(&memoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_ACTIVITY,
            m_pLastSample->AllocatedSizeInBytes,
            &memoryHandle,
            reinterpret_cast<PVOID*>(&m_pFilteredSample));
        if (!NT_SUCCESS(status) || nullptr == m_pFilteredSample)
        {
            TraceError("ACT %!FUNC! WdfMemoryCreate failed %!STATUS!", status);
        }
        else
        {
            // It's safe to memcpy because there is no embedded pointer
            memcpy_s(m_pFilteredSample, m_pLastSample->AllocatedSizeInBytes, m_pLastSample, m_pLastSample->AllocatedSizeInBytes);
        }
    }

    // Get the Marshalled size for a single history record
    if (NT_SUCCESS(status))
    {
        // Set the count to 3, as the History Record contains information about only 
        // the most probable activity (unlike the Activity Data that can represent multiple activities)
        // { Timestamp, ActivityState, Confidence}
        m_pFilteredSample->Count = 3;
        // History Retrieval is not WOW64 compatible and hence will not involve 
        // serializing the collections list. Should Use 
        // CollectionsListGetMarshalledSizeWithoutSerialization  instead of 
        // CollectionsListGetMarshalledSize when dealing with History Collection list.
        m_HistoryMarshalledRecordSize = CollectionsListGetMarshalledSizeWithoutSerialization(m_pFilteredSample);
    }

    // Sensor Properties. This must be called after setting up m_pLastSample and m_HistoryMarshalledRecordSize
    if (NT_SUCCESS(status))
    {
        status = InitializeSensorProperties();
    }

    // Sensor Enumeration Properties.
    if (NT_SUCCESS(status))
    {
        status = InitializeEnumerationProperties();
    }

    // Supported Data-Fields
    if (NT_SUCCESS(status))
    {
        status = InitializeSupportedDataFields();
    }

    // Data field properties
    if (NT_SUCCESS(status))
    {
        status = InitializeDataFieldProperties();
    }

    // Set default threshold
    if (NT_SUCCESS(status))
    {
        const ULONG size = SENSOR_COLLECTION_LIST_SIZE(ACTIVITY_THRESHOLD_COUNT);
        WDF_OBJECT_ATTRIBUTES memoryAttributes = {};
        WDFMEMORY memoryHandle = NULL;

        WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
        memoryAttributes.ParentObject = sensorInstance;
        status = WdfMemoryCreate(&memoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_ACTIVITY,
            size,
            &memoryHandle,
            reinterpret_cast<PVOID*>(&m_pThresholds));
        if (!NT_SUCCESS(status) || nullptr == m_pThresholds)
        {
            TraceError("ACT %!FUNC! WdfMemoryCreate failed %!STATUS!", status);
        }
        else
        {
            SENSOR_COLLECTION_LIST_INIT(m_pThresholds, size);
            m_pThresholds->Count = ACTIVITY_THRESHOLD_COUNT;

            m_pThresholds->List[ACTIVITY_THRESHOLD_SUBSCRIBED_STATES].Key = PKEY_SensorData_SubscribedActivityStates;
            InitPropVariantFromUInt32(Act_Default_SubscribedStates,
                &(m_pThresholds->List[ACTIVITY_THRESHOLD_SUBSCRIBED_STATES].Value));

            m_pThresholds->List[ACTIVITY_THRESHOLD_STREAMING].Key = PKEY_SensorData_ActivityStream;
            InitPropVariantFromBoolean(Act_Default_Streaming,
                &(m_pThresholds->List[ACTIVITY_THRESHOLD_STREAMING].Value));

            m_pThresholds->List[ACTIVITY_THRESHOLD_CONFIDENCE].Key = PKEY_SensorData_ConfidenceThreshold_Percentage;
            InitPropVariantFromUInt16(Act_Default_ConfidenceThreshold_Percentage,
                &(m_pThresholds->List[ACTIVITY_THRESHOLD_CONFIDENCE].Value));
        }
    }

    // Initialize history buffer
    if (NT_SUCCESS(status))
    {
        const ULONG size = sizeof(ActivitySample) * m_HistorySizeInRecords;
        WDF_OBJECT_ATTRIBUTES memoryAttributes = {};
        WDFMEMORY memoryHandle = NULL;

        WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
        memoryAttributes.ParentObject = sensorInstance;
        status = WdfMemoryCreate(&memoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_ACTIVITY,
            size,
            &memoryHandle,
            reinterpret_cast<PVOID*>(&(m_History.pData)));
        if (!NT_SUCCESS(status) || nullptr == m_History.pData)
        {
            TraceError("ACT %!FUNC! WdfMemoryCreate failed %!STATUS!", status);
        }
    }

    // Create event for signaling the history retrieval thread to exit
    if (NT_SUCCESS(status))
    {
        m_ExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_ExitEvent || INVALID_HANDLE_VALUE == m_ExitEvent)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("ACT %!FUNC! Failed to create an event %!STATUS!", status);
        }
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine is the AddDevice entry point for the fake activity client
// driver. This routine is called by the framework in response to AddDevice
// call from the PnP manager. It will create and initialize the device object
// to represent a new instance of the sensor client.
NTSTATUS ActivityDevice::OnDeviceAdd(
    _In_ WDFDRIVER /*driver*/,              // Supplies a handle to the driver object created in DriverEntry
    _Inout_ PWDFDEVICE_INIT pDeviceInit)    // Supplies a pointer to a framework-allocated WDFDEVICE_INIT structure
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Initialize FDO attributes and set up file object with sensor extension
    WDF_OBJECT_ATTRIBUTES fdoAttributes = {};
    ULONG flag = 0;

    WDF_OBJECT_ATTRIBUTES_INIT(&fdoAttributes);
    status = SensorsCxDeviceInitConfig(pDeviceInit, &fdoAttributes, flag);
    if (!NT_SUCCESS(status))
    {
        TraceError("ACT %!FUNC! SensorsCxDeviceInitConfig failed %!STATUS!", status);
    }
    else
    {
        WDF_PNPPOWER_EVENT_CALLBACKS callbacks = {};
        WDFDEVICE device = NULL;

        // Register the PnP callbacks with the framework.
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&callbacks);
        callbacks.EvtDevicePrepareHardware = ActivityDevice::OnPrepareHardware;
        callbacks.EvtDeviceReleaseHardware = ActivityDevice::OnReleaseHardware;
        callbacks.EvtDeviceD0Entry = ActivityDevice::OnD0Entry;
        callbacks.EvtDeviceD0Exit = ActivityDevice::OnD0Exit;
        WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &callbacks);

        // Call the framework to create the device
        status = WdfDeviceCreate(&pDeviceInit, &fdoAttributes, &device);
        if (!NT_SUCCESS(status))
        {
            TraceError("ACT %!FUNC! WdfDeviceCreate failed %!STATUS!", status);
        }
        else
        {
            SENSOR_CONTROLLER_CONFIG sensorConfig = {};

            // Register CLX callback function pointers
            SENSOR_CONTROLLER_CONFIG_INIT(&sensorConfig);
            sensorConfig.DriverIsPowerPolicyOwner = WdfUseDefault;

            sensorConfig.EvtSensorStart = ActivityDevice::OnStart;
            sensorConfig.EvtSensorStop = ActivityDevice::OnStop;
            sensorConfig.EvtSensorGetSupportedDataFields = ActivityDevice::OnGetSupportedDataFields;
            sensorConfig.EvtSensorGetDataInterval = ActivityDevice::OnGetDataInterval;
            sensorConfig.EvtSensorSetDataInterval = ActivityDevice::OnSetDataInterval;
            sensorConfig.EvtSensorGetDataFieldProperties = ActivityDevice::OnGetDataFieldProperties;
            sensorConfig.EvtSensorGetDataThresholds = ActivityDevice::OnGetDataThresholds;
            sensorConfig.EvtSensorSetDataThresholds = ActivityDevice::OnSetDataThresholds;
            sensorConfig.EvtSensorGetProperties = ActivityDevice::OnGetProperties;
            sensorConfig.EvtSensorDeviceIoControl = ActivityDevice::OnIoControl;
            sensorConfig.EvtSensorStartHistory = ActivityDevice::OnStartHistory;
            sensorConfig.EvtSensorStopHistory = ActivityDevice::OnStopHistory;
            sensorConfig.EvtSensorClearHistory = ActivityDevice::OnClearHistory;
            sensorConfig.EvtSensorStartHistoryRetrieval = ActivityDevice::OnStartHistoryRetrieval;
            sensorConfig.EvtSensorCancelHistoryRetrieval = ActivityDevice::OnCancelHistoryRetrieval;

            // Set up power capabilities and IO queues
            status = SensorsCxDeviceInitialize(device, &sensorConfig);
            if (!NT_SUCCESS(status))
            {
                TraceError("ACT %!FUNC! SensorsCxDeviceInitialize failed %!STATUS!", status);
            }
        }
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine is called by the framework when the PnP manager sends an
// IRP_MN_START_DEVICE request to the driver stack. This routine is
// responsible for performing operations that are necessary to make the
// driver's device operational (for e.g. mapping the hardware resources
// into memory).
NTSTATUS ActivityDevice::OnPrepareHardware(
    _In_ WDFDEVICE device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesRaw*/,         // Supplies a handle to a collection of framework resource
                                                // objects. This collection identifies the raw (bus-relative) hardware
                                                // resources that have been assigned to the device.
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Create WDFOBJECT for the sensor
    WDF_OBJECT_ATTRIBUTES sensorAttributes = {};
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&sensorAttributes, ActivityDevice);
    
    // Register sensor instance with clx
    SENSOROBJECT sensorInstance = NULL;
    status = SensorsCxSensorCreate(device, &sensorAttributes, &sensorInstance);
    if (!NT_SUCCESS(status))
    {
        TraceError("ACT %!FUNC! SensorsCxSensorCreate failed %!STATUS!", status);
    }
    else
    {
        PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
        if (nullptr == pDevice)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("ACT %!FUNC! GetActivityContextFromSensorInstance failed %!STATUS!", status);
        }
        else
        {
            // Fill out properties
            status = pDevice->Initialize(device, sensorInstance);
            if (!NT_SUCCESS(status))
            {
                TraceError("ACT %!FUNC! Initialize device object failed %!STATUS!", status);
            }
            else
            {
                SENSOR_CONFIG sensorConfig = {};
                SENSOR_CONFIG_INIT(&sensorConfig);
                sensorConfig.pEnumerationList = pDevice->m_pEnumerationProperties;
                status = SensorsCxSensorInitialize(sensorInstance, &sensorConfig);
                if (!NT_SUCCESS(status))
                {
                    TraceError("ACT %!FUNC! SensorsCxSensorInitialize failed %!STATUS!", status);
                }
            }
        }
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine is called by the framework when the PnP manager is revoking
// ownership of our resources. This may be in response to either
// IRP_MN_STOP_DEVICE or IRP_MN_REMOVE_DEVICE. This routine is responsible for
// performing cleanup of resources allocated in PrepareHardware callback.
// This callback is invoked before passing  the request down to the lower driver.
// This routine will also be invoked by the framework if the prepare hardware
// callback returns a failure.
NTSTATUS ActivityDevice::OnReleaseHardware(
    _In_ WDFDEVICE device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Get sensor instance
    SENSOROBJECT sensorInstance = NULL;
    ULONG sensorInstanceCount = 1;    // only expect 1 sensor instance

    status = SensorsCxDeviceGetSensorList(device, &sensorInstance, &sensorInstanceCount);
    if (!NT_SUCCESS(status) || sensorInstanceCount == 0 || sensorInstance == NULL)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", status);
    }
    else
    {
        PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
        if (nullptr == pDevice)
        {
            status = STATUS_INVALID_PARAMETER;
            TraceError("ACT %!FUNC! GetActivityContextFromSensorInstance failed %!STATUS!", status);
        }
        else
        {
            // Cleanup activity simulator
            if (NULL != pDevice->m_SimulatorInstance)
            {
                PHardwareSimulator pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
                if (nullptr != pSimulator)
                {
                    pSimulator->Deinitialize();
                }
                // Continue tearing down
                WdfObjectDelete(pDevice->m_SimulatorInstance);
                pDevice->m_SimulatorInstance = NULL;
            }

            // Close handle to history retrieval thread
            if (NULL != pDevice->m_hThread)
            {
                SetEvent(pDevice->m_ExitEvent);

                DWORD result = WaitForSingleObjectEx(pDevice->m_hThread, Act_TimeoutForHistoryThread_Ms, FALSE);
                if (WAIT_OBJECT_0 != result)
                {
                    // continue tearing down
                    status = NTSTATUS_FROM_WIN32(result);
                    TraceError("ACT %!FUNC! WaitForSingleObjectEx failed %!STATUS!", status);
                }

                CloseHandle(pDevice->m_hThread);
                pDevice->m_hThread = NULL;
            }

            // Close handle to exit event
            if (NULL != pDevice->m_ExitEvent)
            {
                CloseHandle(pDevice->m_ExitEvent);
                pDevice->m_ExitEvent = NULL;
            }

            // Delete history lock
            if (NULL != pDevice->m_HistoryLock)
            {
                WdfObjectDelete(pDevice->m_HistoryLock);
                pDevice->m_HistoryLock = NULL;
            }

            // Delete lock
            if (NULL != pDevice->m_Lock)
            {
                WdfObjectDelete(pDevice->m_Lock);
                pDevice->m_Lock = NULL;
            }

            // Delete sensor instance.
            if (NULL != pDevice->m_SensorInstance)
            {
                // Memory created by WdfMemoryCreate with m_SensorInstance as parent object will be
                // destroyed automatically when m_SensorInstance is deleted. pDevice will be no longer
                // accessible at beyond this point.
                WdfObjectDelete(pDevice->m_SensorInstance);
                pDevice = nullptr;
            }
        }
    }
 
    SENSOR_FunctionExit(status);
    return status;
}

// This routine is invoked by the framework to program the device to goto 
// D0, which is the working state. The framework invokes callback every
// time the hardware needs to be (re-)initialized.  This includes after
// IRP_MN_START_DEVICE, IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE,
// and IRP_MN_SET_POWER-D0.
NTSTATUS ActivityDevice::OnD0Entry(
    _In_ WDFDEVICE device,                          // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*PreviousState*/)  // WDF_POWER_DEVICE_STATE-typed enumerator that identifies the device
                                                    // power state that the device was in before this transition to D0.
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

     // Get sensor instance
    SENSOROBJECT sensorInstance = NULL;
    ULONG sensorInstanceCount = 1;

    status = SensorsCxDeviceGetSensorList(device, &sensorInstance, &sensorInstanceCount);
    if (!NT_SUCCESS(status) || sensorInstanceCount == 0 || sensorInstance == NULL)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", status);
    }
    else
    {

        PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
        if (nullptr == pDevice)
        {
            status = STATUS_INVALID_PARAMETER;
            TraceError("ACT %!FUNC! GetActivityContextFromSensorInstance failed %!STATUS!", status);
        }
        else
        {
            // Power on sensor
            pDevice->m_PoweredOn = TRUE;
            InitPropVariantFromUInt32(SensorState_Idle, &(pDevice->m_pProperties->List[SENSOR_PROPERTY_STATE].Value));
        }
    }

    SENSOR_FunctionExit(status);
    return status;
}

// This routine is invoked by the framework to program the device to go into
// a certain Dx state. The framework invokes callback every the the device is 
// leaving the D0 state, which happens when the device is stopped, when it is 
// removed, and when it is powered off.
NTSTATUS ActivityDevice::OnD0Exit(
    _In_ WDFDEVICE device,                          // Supplies a handle to the framework device object.
    _In_ WDF_POWER_DEVICE_STATE /*TargetState*/)    // Supplies the device power state which the device 
                                                    // will be put in once the callback is complete.
{
    NTSTATUS status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Get sensor instance
    SENSOROBJECT sensorInstance = NULL;
    ULONG sensorInstanceCount = 1;
    status = SensorsCxDeviceGetSensorList(device, &sensorInstance, &sensorInstanceCount);
    if (!NT_SUCCESS(status) || sensorInstanceCount == 0 || sensorInstance == NULL)
    {
        status = STATUS_INVALID_PARAMETER;
        TraceError("ACT %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", status);
    }
    else
    {

        PActivityDevice pDevice = GetActivityContextFromSensorInstance(sensorInstance);
        if (nullptr == pDevice)
        {
            status = STATUS_INVALID_PARAMETER;
            TraceError("ACT %!FUNC! GetActivityContextFromSensorInstance failed %!STATUS!", status);
        }
        else
        {
            // Power off sensor
            pDevice->m_PoweredOn = FALSE;
        }
    }

    SENSOR_FunctionExit(status);
    return status;
}