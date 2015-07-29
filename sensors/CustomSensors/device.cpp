//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of WDF callback functions 
//    for sample custom sensor driver.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"

#include "Device.tmh"

// Custom sensor unique ID
// {A8B27439-C671-401E-B4EF-43D700A5B411}
//
// TODO: The unique ID below must be set per sensor. A different GUID must be provided for each sensor. Please generate a new GUID.
DEFINE_GUID(GUID_CustomSensorDevice_UniqueID,
    0xa8b27439, 0xc671, 0x401e, 0xb4, 0xef, 0x43, 0xd7, 0x0, 0xa5, 0xb4, 0x11);

// Custom sensor vendor defined type
//
// The following ID is defined by vendors and is unique to a custom sensor type. Each custom sensor driver should define one unique ID.
// 
// TODO: The ID below identifies the custom sensor CO2 emulation sample driver. Please generate a new GUID for each new type of driver.
//
// {4025A865-638C-43AA-A688-98580961EEAE}
DEFINE_GUID(GUID_CustomSensorDevice_VendorDefinedSubTypeID,
    0x4025a865, 0x638c, 0x43aa, 0xa6, 0x88, 0x98, 0x58, 0x9, 0x61, 0xee, 0xae);

// Vendor defined property keys
//
// A property key is defined by vendors for each datafield property a custom sensor driver exposes. Property keys are defined
// per custom sensor driver and is unique to each custom sensor type.
// 
// The following property key represents the CO2 level datafield exposed by this driver.
// In this example only one key is defined, but other drivers may define more than one key by rev'ing up the property key index.
// 
// TODO: The PROPERTYKEY below identifies the custom sensor CO2 level property key. Please generate a new GUID for each new type of driver.
// 
// {74879888-a3cc-45c6-9ea9-058838256433} 1
DEFINE_PROPERTYKEY(PKEY_CustomSensorSampleData_CO2Level,
    0x74879888, 0xa3cc, 0x45c6, 0x9e, 0xa9, 0x5, 0x88, 0x38, 0x25, 0x64, 0x33, 1); //[VT_R4]



// This routine initializes the sensor to its default properties
NTSTATUS CustomSensorDevice::Initialize(
    _In_ WDFDEVICE Device,            // WDFDEVICE object
    _In_ SENSOROBJECT SensorInstance  // SENSOROBJECT for each sensor instance
    )
{
    ULONG Size = 0;
    WDF_OBJECT_ATTRIBUTES MemoryAttributes;
    WDFMEMORY MemoryHandle = NULL;
    FILETIME Time = {};
    WDF_OBJECT_ATTRIBUTES TimerAttributes;
    WDF_TIMER_CONFIG TimerConfig;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Store device and instance
    m_FxDevice = Device;
    m_SensorInstance = SensorInstance;
    m_Started = FALSE;

    // Initialize the CO2 simulator
    HardwareSimulator::Initialize(Device, &m_SimulatorInstance);

    // Create Lock
    Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
    if (!NT_SUCCESS(Status))
    {
        TraceError("CSTM %!FUNC! WdfWaitLockCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Create timer object for polling sensor samples
    WDF_TIMER_CONFIG_INIT(&TimerConfig, CustomSensorDevice::OnTimerExpire);
    WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
    TimerAttributes.ParentObject = SensorInstance;
    TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_Timer);
    if (!NT_SUCCESS(Status))
    {
        TraceError("CSTM %!FUNC! WdfTimerCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Sensor Enumeration Properties
    Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ENUMERATION_PROPERTIES_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSORV2_POOL_TAG_CUSTOM_SENSOR,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pEnumerationProperties));
    if (!NT_SUCCESS(Status) || nullptr == m_pEnumerationProperties)
    {
        TraceError("CSTM %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
    m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

    // The sensor type must be GUID_SensorType_Custom, the driver must also define and "vendor defined subtype"
    m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
    InitPropVariantFromCLSID(GUID_SensorType_Custom,
                             &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

    m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
    InitPropVariantFromString(L"Microsoft", 
                             &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

    m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
    InitPropVariantFromString(L"CO2 based sample Custom sensor V2", 
                             &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));
    
    m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
    InitPropVariantFromCLSID(GUID_CustomSensorDevice_UniqueID, 
                             &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));

    m_pEnumerationProperties->List[SENSOR_CATEGORY].Key = DEVPKEY_Sensor_Category;
    InitPropVariantFromCLSID(GUID_SensorCategory_Other, 
                             &(m_pEnumerationProperties->List[SENSOR_CATEGORY].Value));

    m_pEnumerationProperties->List[SENSOR_VENDOR_DEFINED_TYPE].Key = DEVPKEY_Sensor_VendorDefinedSubType;
    InitPropVariantFromCLSID(GUID_CustomSensorDevice_VendorDefinedSubTypeID,
                             &(m_pEnumerationProperties->List[SENSOR_VENDOR_DEFINED_TYPE].Value));

    // Supported Data-Fields
    Size = SENSOR_PROPERTY_LIST_SIZE(CSTM_DATA_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSORV2_POOL_TAG_CUSTOM_SENSOR,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pSupportedDataFields));
    if (!NT_SUCCESS(Status) || nullptr == m_pSupportedDataFields)
    {
        TraceError("CSTM %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
    m_pSupportedDataFields->Count = CSTM_DATA_COUNT;

    m_pSupportedDataFields->List[CSTM_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
    m_pSupportedDataFields->List[CSTM_DATA_CO2_LEVEL_PERCENT] = PKEY_CustomSensorSampleData_CO2Level;

    // Data
    Size = SENSOR_COLLECTION_LIST_SIZE(CSTM_DATA_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSORV2_POOL_TAG_CUSTOM_SENSOR,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pData));
    if (!NT_SUCCESS(Status) || nullptr == m_pData)
    {
        TraceError("CSTM %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
    m_pData->Count = CSTM_DATA_COUNT;

    m_pData->List[CSTM_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
    GetSystemTimePreciseAsFileTime(&Time);
    InitPropVariantFromFileTime(&Time, &(m_pData->List[CSTM_DATA_TIMESTAMP].Value));

    // Initialize the sample, at this point of time the sensor is not started yet,
    // So, initialize the sample to a default value
    m_pData->List[CSTM_DATA_CO2_LEVEL_PERCENT].Key = PKEY_CustomSensorSampleData_CO2Level;
    InitPropVariantFromFloat(CustomSensorDevice_Minimum_CO2Level, &(m_pData->List[CSTM_DATA_CO2_LEVEL_PERCENT].Value));

    // Sensor Properties
    m_Interval = Cstm_Default_MinDataInterval_Ms;

    Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_COMMON_PROPERTIES_COUNT);

    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSORV2_POOL_TAG_CUSTOM_SENSOR,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pProperties));
    if (!NT_SUCCESS(Status) || nullptr == m_pProperties)
    {
        TraceError("CSTM %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
    m_pProperties->Count = SENSOR_COMMON_PROPERTIES_COUNT;

    m_pProperties->List[SENSOR_PROPERTY_STATE].Key = PKEY_Sensor_State;
    InitPropVariantFromUInt32(SensorState_Initializing, 
                              &(m_pProperties->List[SENSOR_PROPERTY_STATE].Value));

    m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
    InitPropVariantFromUInt32(Cstm_Default_MinDataInterval_Ms, 
                              &(m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Value));

    m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
    InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData), 
                              &(m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Value));

    m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Key = PKEY_Sensor_Type;
    InitPropVariantFromCLSID(GUID_SensorType_Custom, 
                                 &(m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Value));

    // Data filed properties
    Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_DATA_FIELD_PROPERTY_COUNT);
    
    MemoryHandle = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;
    Status = WdfMemoryCreate(&MemoryAttributes, 
                             PagedPool, 
                             SENSORV2_POOL_TAG_CUSTOM_SENSOR,
                             Size,
                             &MemoryHandle,
                             reinterpret_cast<PVOID*>(&m_pDataFieldProperties));
    if (!NT_SUCCESS(Status) || nullptr == m_pDataFieldProperties)
    {
        TraceError("CSTM %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_COLLECTION_LIST_INIT(m_pDataFieldProperties, Size);
    m_pDataFieldProperties->Count = SENSOR_DATA_FIELD_PROPERTY_COUNT;

    m_pDataFieldProperties->List[SENSOR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
    InitPropVariantFromFloat((float)CustomSensorDevice_Resolution,
                             &(m_pDataFieldProperties->List[SENSOR_RESOLUTION].Value));

    m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
    InitPropVariantFromFloat(CustomSensorDevice_Minimum_CO2Level,
                             &(m_pDataFieldProperties->List[SENSOR_MIN_RANGE].Value));

    m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
    InitPropVariantFromFloat(CustomSensorDevice_Maximum_CO2Level,
                             &(m_pDataFieldProperties->List[SENSOR_MAX_RANGE].Value));


    // Reset the FirstSample flag
    m_FirstSample = TRUE;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// This routine is the AddDevice entry point for the custom sensor client
// driver. This routine is called by the framework in response to AddDevice
// call from the PnP manager. It will create and initialize the device object
// to represent a new instance of the sensor client.
NTSTATUS CustomSensorDevice::OnDeviceAdd(
    _In_ WDFDRIVER /*Driver*/,            // Supplies a handle to the driver object created in DriverEntry
    _Inout_ PWDFDEVICE_INIT pDeviceInit   // Supplies a pointer to a framework-allocated WDFDEVICE_INIT structure
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
        TraceError("CSTM %!FUNC! SensorsCxDeviceInitConfig failed %!STATUS!", Status);
        goto Exit;
    }
    
    // Register the PnP callbacks with the framework.
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&Callbacks);
    Callbacks.EvtDevicePrepareHardware = CustomSensorDevice::OnPrepareHardware;
    Callbacks.EvtDeviceReleaseHardware = CustomSensorDevice::OnReleaseHardware;
    Callbacks.EvtDeviceD0Entry = CustomSensorDevice::OnD0Entry;
    Callbacks.EvtDeviceD0Exit = CustomSensorDevice::OnD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &Callbacks);
    
    // Call the framework to create the device
    Status = WdfDeviceCreate(&pDeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("CSTM %!FUNC! WdfDeviceCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Register CLX callback function pointers
    SENSOR_CONTROLLER_CONFIG_INIT(&SensorConfig);
    SensorConfig.DriverIsPowerPolicyOwner = WdfUseDefault;

    SensorConfig.EvtSensorStart                     = CustomSensorDevice::OnStart;
    SensorConfig.EvtSensorStop                      = CustomSensorDevice::OnStop;
    SensorConfig.EvtSensorGetSupportedDataFields    = CustomSensorDevice::OnGetSupportedDataFields;
    SensorConfig.EvtSensorGetDataInterval           = CustomSensorDevice::OnGetDataInterval;
    SensorConfig.EvtSensorSetDataInterval           = CustomSensorDevice::OnSetDataInterval;
    SensorConfig.EvtSensorGetDataFieldProperties    = CustomSensorDevice::OnGetDataFieldProperties;
    SensorConfig.EvtSensorGetDataThresholds         = CustomSensorDevice::OnGetDataThresholds;
    SensorConfig.EvtSensorSetDataThresholds         = CustomSensorDevice::OnSetDataThresholds;
    SensorConfig.EvtSensorGetProperties             = CustomSensorDevice::OnGetProperties;
    SensorConfig.EvtSensorDeviceIoControl           = CustomSensorDevice::OnIoControl;

    // Set up power capabilities and IO queues
    Status = SensorsCxDeviceInitialize(Device, &SensorConfig);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("CSTM %!FUNC! SensorsCxDeviceInitialize failed %!STATUS!", Status);
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
CustomSensorDevice::OnPrepareHardware(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesRaw*/,         // Supplies a handle to a collection of framework resource
                                                // objects. This collection identifies the raw (bus-relative) hardware
                                                // resources that have been assigned to the device.
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
    
{
    PCustomSensorDevice pDevice = nullptr;
    WDF_OBJECT_ATTRIBUTES SensorAttr = {};
    SENSOR_CONFIG SensorConfig = {};
    SENSOROBJECT SensorInstance = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Construct sensor instance

    // Create WDFOBJECT for the sensor
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&SensorAttr, CustomSensorDevice);
    
    // Register sensor instance with clx
    Status = SensorsCxSensorCreate(Device, &SensorAttr, &SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("CSTM %!FUNC! SensorsCxSensorCreate failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("CSTM %!FUNC! GetCustomSensorContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Fill out properties
    Status = pDevice->Initialize(Device, SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("CSTM %!FUNC! Initialize device object failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_CONFIG_INIT(&SensorConfig);
    SensorConfig.pEnumerationList = pDevice->m_pEnumerationProperties;
    Status = SensorsCxSensorInitialize(SensorInstance, &SensorConfig);
    if (!NT_SUCCESS(Status))
    {
        TraceError("CSTM %!FUNC! SensorsCxSensorInitialize failed %!STATUS!", Status);
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
NTSTATUS
CustomSensorDevice::OnReleaseHardware(
    _In_ WDFDEVICE Device,                       // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)   // Supplies a handle to a collection of framework
                                                 // resource objects. This collection identifies the translated
                                                 // (system-physical) hardware resources that have been assigned to the
                                                 // device. The resources appear from the CPU's point of view.
{
    PHardwareSimulator pSimulator = nullptr;
    PCustomSensorDevice pDevice = nullptr;
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
        TraceError("CSTM %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! GetCustomSensorContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Delete lock
    if (pDevice->m_Lock)
    {
        WdfObjectDelete(pDevice->m_Lock);
        pDevice->m_Lock = NULL;
    }    
    
    // Cleanup the CO2 simulator
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("CSTM %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

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
CustomSensorDevice::OnD0Entry(
    _In_ WDFDEVICE Device,                          // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*PreviousState*/)  // WDF_POWER_DEVICE_STATE-typed enumerator that identifies
                                                    // the device power state that the device was in before this transition to D0
{
    PCustomSensorDevice pDevice;
    SENSOROBJECT SensorInstance = nullptr;
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
        TraceError("CSTM %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! GetCustomSensorContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Power on sensor
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
CustomSensorDevice::OnD0Exit(
    _In_ WDFDEVICE Device,                       // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*TargetState*/) // Supplies the device power state which the device will be put
                                                 // in once the callback is complete
{
    PCustomSensorDevice pDevice;
    SENSOROBJECT SensorInstance = nullptr;
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
        TraceError("CSTM %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetCustomSensorContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("CSTM %!FUNC! GetCustomSensorContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Power off sensor
    pDevice->m_PoweredOn = FALSE;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}
