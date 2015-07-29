//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of WDF callback functions 
//    for pedometer driver.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "HardwareSimulator.h"

#include "Device.tmh"

// Pedometer Unique ID
// {E3CF8012-53B7-4E3F-9F4E-86C1BCC1B938}
//
// TODO: The unique ID below must be set per sensor. A different GUID must be provided for each sensor. Please generate a new GUID.
DEFINE_GUID(GUID_PedometerDevice_UniqueID,
    0xe3cf8012, 0x53b7, 0x4e3f, 0x9f, 0x4e, 0x86, 0xc1, 0xbc, 0xc1, 0xb9, 0x38);


// This routine initializes the sensor to its default properties
NTSTATUS
PedometerDevice::Initialize(
    _In_ WDFDEVICE Device, // WDFDEVICE object
    _In_ SENSOROBJECT SensorInstance // SENSOROBJECT for each sensor instance
    )
{
    ULONG Size = 0;
    WDF_OBJECT_ATTRIBUTES MemoryAttributes;
    WDFMEMORY MemoryHandle = NULL;
    FILETIME Time = {};
    WDF_OBJECT_ATTRIBUTES TimerAttributes;
    WDF_TIMER_CONFIG TimerConfig;
    NTSTATUS Status = STATUS_SUCCESS;
    PHardwareSimulator pSimulator = nullptr;
    ULONG HistorySizeInRecords = 0;

    SENSOR_FunctionEnter();

    // Store device and instance
    m_FxDevice = Device;
    m_SensorInstance = SensorInstance;
    m_Started = FALSE;
    m_HistoryRetrievalStarted = FALSE;

    // Initialize the pedometer simulator
    Status = HardwareSimulator::Initialize(Device, &m_SimulatorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! HardwareSimulator::Initialize failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Create Lock
    Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! WdfWaitLockCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Create timer object for polling sensor samples
    WDF_TIMER_CONFIG_INIT(&TimerConfig, PedometerDevice::OnTimerExpire);
    WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
    TimerAttributes.ParentObject = SensorInstance;
    TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_Timer);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! WdfTimerCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Supported Data-Fields
    Size = SENSOR_PROPERTY_LIST_SIZE(PEDOMETER_DATAFIELD_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSOR_POOL_TAG_PEDOMETER,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pSupportedDataFields));
    if (!NT_SUCCESS(Status) || nullptr == m_pSupportedDataFields)
    {
        TraceError("PED %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
    m_pSupportedDataFields->Count = PEDOMETER_DATAFIELD_COUNT;

    m_pSupportedDataFields->List[PEDOMETER_DATAFIELD_TIMESTAMP] = PKEY_SensorData_Timestamp;
    m_pSupportedDataFields->List[PEDOMETER_DATAFIELD_FIRST_AFTER_RESET] = PKEY_SensorData_PedometerReset;
    m_pSupportedDataFields->List[PEDOMETER_DATAFIELD_STEP_TYPE] = PKEY_SensorData_PedometerStepType;
    m_pSupportedDataFields->List[PEDOMETER_DATAFIELD_STEP_COUNT] = PKEY_SensorData_PedometerStepCount;
    m_pSupportedDataFields->List[PEDOMETER_DATAFIELD_STEP_DURATION] = PKEY_SensorData_PedometerStepDuration_Ms;

    // Data
    Size = SENSOR_COLLECTION_LIST_SIZE(PEDOMETER_DATA_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSOR_POOL_TAG_PEDOMETER,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pData));
    if (!NT_SUCCESS(Status) || nullptr == m_pData)
    {
        TraceError("PED %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
    m_pData->Count = PEDOMETER_DATA_COUNT;

    m_pData->List[PEDOMETER_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
    GetSystemTimePreciseAsFileTime(&Time);
    InitPropVariantFromFileTime(&Time, &(m_pData->List[PEDOMETER_DATA_TIMESTAMP].Value));

    m_pData->List[PEDOMETER_DATA_FIRST_AFTER_RESET].Key = PKEY_SensorData_PedometerReset;
    InitPropVariantFromBoolean(FALSE, &(m_pData->List[PEDOMETER_DATA_FIRST_AFTER_RESET].Value));

    m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_TYPE].Key = PKEY_SensorData_PedometerStepType;
    InitPropVariantFromUInt32(static_cast<ULONG>(PedometerStepType_Unknown), &(m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_TYPE].Value));

    m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_COUNT].Key = PKEY_SensorData_PedometerStepCount;
    InitPropVariantFromUInt32(600, &(m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_COUNT].Value));

    m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_DURATION].Key = PKEY_SensorData_PedometerStepDuration_Ms;
    InitPropVariantFromInt64(123, &(m_pData->List[PEDOMETER_DATA_UNKNOWN_STEP_DURATION].Value));

    m_pData->List[PEDOMETER_DATA_WALKING_STEP_TYPE].Key = PKEY_SensorData_PedometerStepType;
    InitPropVariantFromUInt32(static_cast<ULONG>(PedometerStepType_Walking), &(m_pData->List[PEDOMETER_DATA_WALKING_STEP_TYPE].Value));

    m_pData->List[PEDOMETER_DATA_WALKING_STEP_COUNT].Key = PKEY_SensorData_PedometerStepCount;
    InitPropVariantFromUInt32(700, &(m_pData->List[PEDOMETER_DATA_WALKING_STEP_COUNT].Value));

    m_pData->List[PEDOMETER_DATA_WALKING_STEP_DURATION].Key = PKEY_SensorData_PedometerStepDuration_Ms;
    InitPropVariantFromInt64(456, &(m_pData->List[PEDOMETER_DATA_WALKING_STEP_DURATION].Value));

    m_pData->List[PEDOMETER_DATA_RUNNING_STEP_TYPE].Key = PKEY_SensorData_PedometerStepType;
    InitPropVariantFromUInt32(static_cast<ULONG>(PedometerStepType_Running), &(m_pData->List[PEDOMETER_DATA_RUNNING_STEP_TYPE].Value));

    m_pData->List[PEDOMETER_DATA_RUNNING_STEP_COUNT].Key = PKEY_SensorData_PedometerStepCount;
    InitPropVariantFromUInt32(800, &(m_pData->List[PEDOMETER_DATA_RUNNING_STEP_COUNT].Value));

    m_pData->List[PEDOMETER_DATA_RUNNING_STEP_DURATION].Key = PKEY_SensorData_PedometerStepDuration_Ms;
    InitPropVariantFromInt64(789, &(m_pData->List[PEDOMETER_DATA_RUNNING_STEP_DURATION].Value));

    m_LastSample.Timestamp = Time;
    m_LastSample.UnknownStepCount = 0;
    m_LastSample.UnknownStepDurationMs = 0;
    m_LastSample.WalkingStepCount = 0;
    m_LastSample.WalkingStepDurationMs = 0;
    m_LastSample.RunningStepCount = 0;
    m_LastSample.RunningStepDurationMs = 0;
    m_LastSample.IsFirstAfterReset = FALSE;

    // Get the History Size to populate 'PKEY_SensorHistory_MaxSize_Bytes'
    // Typically the size needed to store a history record on the hardware is 
    // smaller than the size needed to represent a history record as a 
    // SENSOR_COLLECTION_LIST (collection of SENSOR_VALUE_PAIRs)
    // To be able to accurately represent the size of the history on the 
    // hardware (simulator in this case), get the number of records that the HW  
    // can store and multiply it with the marshalled size of 
    // SENSOR_COLLECTION_LIST needed to represent a single record.

    HistorySizeInRecords = pSimulator->GetHistorySizeInRecords();
    m_HistorySupported = (HistorySizeInRecords > 0) ? TRUE : FALSE;

    // Pedometer History format is exactly same as it's data sample.
    // so, we can simply reuse the 'm_pData' to compute the marshalled size
    // History Retrieval is not WOW64 compatible and hence will not involve 
    // serializing the collections list. Should Use 
    // CollectionsListGetMarshalledSizeWithoutSerialization  instead of 
    // CollectionsListGetMarshalledSize when dealing with History Collection list.
    m_HistoryMarshalledRecordSize = CollectionsListGetMarshalledSizeWithoutSerialization(m_pData);

    // Sensor Enumeration Properties
    Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ENUMERATION_PROPERTIES_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes,
        PagedPool,
        SENSOR_POOL_TAG_PEDOMETER,
        Size,
        &MemoryHandle,
        reinterpret_cast<PVOID*>(&m_pEnumerationProperties));
    if (!NT_SUCCESS(Status) || nullptr == m_pEnumerationProperties)
    {
        TraceError("PED %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
    m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

    m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
    InitPropVariantFromCLSID(GUID_SensorType_Pedometer,
        &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

    m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
    InitPropVariantFromString(L"Microsoft",
        &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

    m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
    InitPropVariantFromString(L"PEDOMETER",
        &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

    m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
    InitPropVariantFromCLSID(GUID_PedometerDevice_UniqueID,
        &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));

    m_pEnumerationProperties->List[SENSOR_CATEGORY].Key = DEVPKEY_Sensor_Category;
    InitPropVariantFromCLSID(GUID_SensorCategory_Motion,
        &(m_pEnumerationProperties->List[SENSOR_CATEGORY].Value));

    m_pEnumerationProperties->List[SENSOR_POWER].Key = PKEY_Sensor_Power_Milliwatts;
    InitPropVariantFromFloat(Pedometer_Default_Power_Milliwatts,
        &(m_pEnumerationProperties->List[SENSOR_POWER].Value));

    m_pEnumerationProperties->List[SENSOR_MAX_HISTORYSIZE].Key = PKEY_SensorHistory_MaxSize_Bytes;
    InitPropVariantFromUInt32(((FALSE != m_HistorySupported) ?
        (SENSOR_COLLECTION_LIST_HEADER_SIZE + ((m_HistoryMarshalledRecordSize - SENSOR_COLLECTION_LIST_HEADER_SIZE) * HistorySizeInRecords)) :
        0),
        &(m_pEnumerationProperties->List[SENSOR_MAX_HISTORYSIZE].Value));

    m_pEnumerationProperties->List[SENSOR_SUPPORTED_STEPTYPES].Key = PKEY_SensorData_SupportedStepTypes;
    InitPropVariantFromUInt32(PedometerStepType_Unknown | PedometerStepType_Walking | PedometerStepType_Running,
        &(m_pEnumerationProperties->List[SENSOR_SUPPORTED_STEPTYPES].Value));

    // Sensor Properties
    m_Interval = Pedometer_Default_MinDataInterval_Ms;

    Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_COMMON_PROPERTIES_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSOR_POOL_TAG_PEDOMETER,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pProperties));
    if (!NT_SUCCESS(Status) || nullptr == m_pProperties)
    {
        TraceError("PED %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
    m_pProperties->Count = SENSOR_COMMON_PROPERTIES_COUNT;

    m_pProperties->List[SENSOR_PROPERTY_STATE].Key = PKEY_Sensor_State;
    InitPropVariantFromUInt32(SensorState_Initializing, 
                              &(m_pProperties->List[SENSOR_PROPERTY_STATE].Value));

    m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
    InitPropVariantFromUInt32(Pedometer_Default_MinDataInterval_Ms, 
                              &(m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Value));

    m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
    InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData), 
                              &(m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Value));

    m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Key = PKEY_Sensor_Type;
    InitPropVariantFromCLSID(GUID_SensorType_Pedometer, 
                                 &(m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Value));

    m_pProperties->List[SENSOR_PROPERTY_SENSOR_POWER].Key = PKEY_Sensor_Power_Milliwatts;
    InitPropVariantFromFloat(Pedometer_Default_Power_Milliwatts,
                                &(m_pProperties->List[SENSOR_PROPERTY_SENSOR_POWER].Value));

    m_pProperties->List[SENSOR_PROPERTY_MAX_HISTORYSIZE].Key = PKEY_SensorHistory_MaxSize_Bytes;
    InitPropVariantFromUInt32(((FALSE != m_HistorySupported) ?
                                (SENSOR_COLLECTION_LIST_HEADER_SIZE + ((m_HistoryMarshalledRecordSize - SENSOR_COLLECTION_LIST_HEADER_SIZE) * HistorySizeInRecords)) :
                                0),
                                &(m_pProperties->List[SENSOR_PROPERTY_MAX_HISTORYSIZE].Value));

    m_pProperties->List[SENSOR_PROPERTY_HISTORY_INTERVAL].Key = PKEY_SensorHistory_Interval_Ms;
    InitPropVariantFromUInt32(pSimulator->GetHistoryIntervalInMs(),
                                &(m_pProperties->List[SENSOR_PROPERTY_HISTORY_INTERVAL].Value));

    m_pProperties->List[SENSOR_PROPERTY_MAX_HISTROYRECORDSIZE].Key = PKEY_SensorHistory_MaximumRecordSize_Bytes;
    InitPropVariantFromUInt32(m_HistoryMarshalledRecordSize,
        &(m_pProperties->List[SENSOR_PROPERTY_MAX_HISTROYRECORDSIZE].Value));

    m_pProperties->List[SENSOR_PROPERTY_SUPPORTED_STEPTYPES].Key = PKEY_SensorData_SupportedStepTypes;
    InitPropVariantFromUInt32(PedometerStepType_Unknown | PedometerStepType_Walking | PedometerStepType_Running,
        &(m_pProperties->List[SENSOR_PROPERTY_SUPPORTED_STEPTYPES].Value));

    // Data field properties
    Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_DATA_FIELD_PROPERTY_COUNT);
    
    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSOR_POOL_TAG_PEDOMETER,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pDataFieldProperties));
    if (!NT_SUCCESS(Status) || nullptr == m_pDataFieldProperties)
    {
        TraceError("PED %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
    m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

    m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
    InitPropVariantFromInt64(PedometerDevice_StepCount_Resolution,
                             &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

    m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
    InitPropVariantFromUInt32(PedometerDevice_StepCount_Minimum,
                             &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

    m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
    InitPropVariantFromUInt32(PedometerDevice_StepCount_Maximum,
                             &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));

    // Set default threshold
    m_FirstSample = TRUE;

    Size = SENSOR_COLLECTION_LIST_SIZE(PEDOMETER_THRESHOLD_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes,
                             PagedPool,
                             SENSOR_POOL_TAG_PEDOMETER,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pThresholds));
    if (!NT_SUCCESS(Status) || nullptr == m_pThresholds)
    {
        TraceError("PED %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
    m_pThresholds->Count = PEDOMETER_THRESHOLD_COUNT;

    m_pThresholds->List[PEDOMETER_THRESHOLD_STEP_COUNT].Key = PKEY_SensorData_PedometerStepCount;
    InitPropVariantFromUInt32(Pedometer_Default_Threshold_StepCount,
                             &(m_pThresholds->List[PEDOMETER_THRESHOLD_STEP_COUNT].Value));

    m_CachedThreshold = Pedometer_Default_Threshold_StepCount;
Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// This routine is the AddDevice entry point for the pedometer client
// driver. This routine is called by the framework in response to AddDevice
// call from the PnP manager. It will create and initialize the device object
// to represent a new instance of the sensor client.
NTSTATUS
PedometerDevice::OnDeviceAdd(
    _In_ WDFDRIVER /*Driver*/, // Supplies a handle to the driver object created in DriverEntry
    _Inout_ PWDFDEVICE_INIT pDeviceInit // Supplies a pointer to a framework-allocated WDFDEVICE_INIT structure
    )
{
    WDF_PNPPOWER_EVENT_CALLBACKS Callbacks;
    WDFDEVICE Device = nullptr;
    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    ULONG Flag = 0;
    SENSOR_CONTROLLER_CONFIG SensorConfig;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    WDF_OBJECT_ATTRIBUTES_INIT(&FdoAttributes);

    // Initialize FDO attributes and set up file object with sensor extension
    Status = SensorsCxDeviceInitConfig(pDeviceInit, &FdoAttributes, Flag);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("PED %!FUNC! SensorsCxDeviceInitConfig failed %!STATUS!", Status);
        goto Exit;
    }
    
    // Register the PnP callbacks with the framework.
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&Callbacks);
    Callbacks.EvtDevicePrepareHardware = PedometerDevice::OnPrepareHardware;
    Callbacks.EvtDeviceReleaseHardware = PedometerDevice::OnReleaseHardware;
    Callbacks.EvtDeviceD0Entry = PedometerDevice::OnD0Entry;
    Callbacks.EvtDeviceD0Exit = PedometerDevice::OnD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &Callbacks);
    
    // Call the framework to create the device
    Status = WdfDeviceCreate(&pDeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("PED %!FUNC! WdfDeviceCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Register CLX callback function pointers
    SENSOR_CONTROLLER_CONFIG_INIT(&SensorConfig);
    SensorConfig.DriverIsPowerPolicyOwner = WdfUseDefault;

    SensorConfig.EvtSensorStart                     = PedometerDevice::OnStart;
    SensorConfig.EvtSensorStop                      = PedometerDevice::OnStop;
    SensorConfig.EvtSensorGetSupportedDataFields    = PedometerDevice::OnGetSupportedDataFields;
    SensorConfig.EvtSensorGetDataInterval           = PedometerDevice::OnGetDataInterval;
    SensorConfig.EvtSensorSetDataInterval           = PedometerDevice::OnSetDataInterval;
    SensorConfig.EvtSensorGetDataFieldProperties    = PedometerDevice::OnGetDataFieldProperties;
    SensorConfig.EvtSensorGetDataThresholds         = PedometerDevice::OnGetDataThresholds;
    SensorConfig.EvtSensorSetDataThresholds         = PedometerDevice::OnSetDataThresholds;
    SensorConfig.EvtSensorGetProperties             = PedometerDevice::OnGetProperties;
    SensorConfig.EvtSensorDeviceIoControl           = PedometerDevice::OnIoControl;
    SensorConfig.EvtSensorStartHistory              = PedometerDevice::OnStartHistory;
    SensorConfig.EvtSensorStopHistory               = PedometerDevice::OnStopHistory;
    SensorConfig.EvtSensorClearHistory              = PedometerDevice::OnClearHistory;
    SensorConfig.EvtSensorStartHistoryRetrieval     = PedometerDevice::OnStartHistoryRetrieval;
    SensorConfig.EvtSensorCancelHistoryRetrieval    = PedometerDevice::OnCancelHistoryRetrieval;

    // Set up power capabilities and IO queues
    Status = SensorsCxDeviceInitialize(Device, &SensorConfig);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("PED %!FUNC! SensorsCxDeviceInitialize failed %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// This routine is called by the framework when the PnP manager sends an
// IRP_MN_START_DEVICE request to the driver stack. This routine is
// responsible for performing operations that are necessary to make the
// driver's device operational (for e.g. mapping the hardware resources
// into memory).
NTSTATUS
PedometerDevice::OnPrepareHardware(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesRaw*/,         // Supplies a handle to a collection of framework resource
                                                // objects. This collection identifies the raw (bus-relative) hardware
                                                // resources that have been assigned to the device.
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
{
    PPedometerDevice pDevice = nullptr;
    WDF_OBJECT_ATTRIBUTES SensorAttr = {};
    SENSOR_CONFIG SensorConfig = {};
    SENSOROBJECT SensorInstance = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Construct sensor instance

    // Create WDFOBJECT for the sensor
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&SensorAttr, PedometerDevice);
    
    // Register sensor instance with clx

    Status = SensorsCxSensorCreate(Device, &SensorAttr, &SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! SensorsCxSensorCreate failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetPedometerContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Device initialization

    Status = pDevice->Initialize(Device, SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! Initialize device object failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_CONFIG_INIT(&SensorConfig);
    SensorConfig.pEnumerationList = pDevice->m_pEnumerationProperties;
    Status = SensorsCxSensorInitialize(SensorInstance, &SensorConfig);
    if (!NT_SUCCESS(Status))
    {
        TraceError("PED %!FUNC! SensorsCxSensorInitialize failed %!STATUS!", Status);
        goto Exit;
    }

Exit:
    SENSOR_FunctionExit(Status);

    return Status;
}



// This routine is called by the framework when the PnP manager is revoking
// ownership of our resources. This may be in response to either
// IRP_MN_STOP_DEVICE or IRP_MN_REMOVE_DEVICE. This routine is responsible for
// performing cleanup of resources allocated in PrepareHardware callback.
// This callback is invoked before passing  the request down to the lower driver.
// This routine will also be invoked by the framework if the prepare hardware
// callback returns a failure.
//
// Argument:
//      Device: IN: Supplies a handle to the framework device object
//      ResourcesTranslated: IN: Supplies a handle to a collection of framework
//          resource objects. This collection identifies the translated
//          (system-physical) hardware resources that have been assigned to the
//          device. The resources appear from the CPU's point of view.
//
// Return Value:
//      NTSTATUS code
//------------------------------------------------------------------------------
NTSTATUS
PedometerDevice::OnReleaseHardware(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
{
    PHardwareSimulator pSimulator = nullptr;
    PPedometerDevice pDevice = nullptr;
    SENSOROBJECT SensorInstance = nullptr;
    ULONG SensorInstanceCount = 1;    // only expect 1 sensor instance
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Get sensor instance
    Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) ||
        0 == SensorInstanceCount ||
        NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! GetPedometerContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("PED %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Close handle to history retrieval thread
    if (NULL != pDevice->m_hThread)
    {
        pSimulator->SignalReadCancellation();

        DWORD result = WaitForSingleObjectEx(pDevice->m_hThread, Pedometer_TimeoutForHistoryThread_Ms, FALSE);
        if (WAIT_OBJECT_0 != result)
        {
            TraceError("PED %!FUNC! WaitForSingleObjectEx failed with error %d", result);
            goto Exit;
        }

        CloseHandle(pDevice->m_hThread);
        pDevice->m_hThread = NULL;
    }


    // Delete lock
    if (NULL != pDevice->m_Lock)
    {
        WdfObjectDelete(pDevice->m_Lock);
        pDevice->m_Lock = NULL;
    }    
    
    // Cleanup the pedometer simulator
    pSimulator->Cleanup();

    // Delete hardware simulator instance
    if (NULL != pDevice->m_SimulatorInstance)
    {
        WdfObjectDelete(pDevice->m_SimulatorInstance);
        pDevice->m_SimulatorInstance = NULL;
    }

    // Delete sensor instance
    if (NULL != pDevice->m_SensorInstance)
    {
        WdfObjectDelete(pDevice->m_SensorInstance);
    }
  
Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// This routine is invoked by the framework to program the device to goto 
// D0, which is the working state. The framework invokes callback every
// time the hardware needs to be (re-)initialized.  This includes after
// IRP_MN_START_DEVICE, IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE,
// and IRP_MN_SET_POWER-D0.
NTSTATUS
PedometerDevice::OnD0Entry(
    _In_ WDFDEVICE Device,                          // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*PreviousState*/)  // WDF_POWER_DEVICE_STATE-typed enumerator that identifies
                                                    // the device power state that the device was in before this transition to D0
{
    PPedometerDevice pDevice;
    SENSOROBJECT SensorInstance = NULL;
    ULONG SensorInstanceCount = 1;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

     // Get sensor instance
    Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) ||
        0 == SensorInstanceCount ||
        NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! GetPedometerContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    //
    // Power on sensor
    //
    pDevice->m_PoweredOn = TRUE;
    InitPropVariantFromUInt32(SensorState_Idle, 
                              &(pDevice->m_pProperties->List[SENSOR_PROPERTY_STATE].Value));

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// This routine is invoked by the framework to program the device to go into
// a certain Dx state. The framework invokes callback every the the device is 
// leaving the D0 state, which happens when the device is stopped, when it is 
// removed, and when it is powered off.
NTSTATUS
PedometerDevice::OnD0Exit(
    _In_ WDFDEVICE Device,                       // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*TargetState*/) // Supplies the device power state which the device will be put
                                                 // in once the callback is complete
{
    PPedometerDevice pDevice;
    SENSOROBJECT SensorInstance = NULL;
    ULONG SensorInstanceCount = 1;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Get sensor instance
    Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) ||
        0 == SensorInstanceCount ||
        NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetPedometerContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("PED %!FUNC! GetPedometerContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    //
    // Power on sensor
    //
    pDevice->m_PoweredOn = FALSE;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}
