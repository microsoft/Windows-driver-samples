//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of WDF callback functions 
//    for sample simple device orientation sensor driver.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"

#include "Device.tmh"

// Simple Device Orientation Sample Sensors Unique ID
// DO NOT REUSE THIS GUID, CREATE A NEW GUID
// {57AB5189-3D73-4E4D-AFEB-019A5CFB8F05}
DEFINE_GUID(GUID_SdoDevice_UniqueID,
    0x57ab5189, 0x3d73, 0x4e4d, 0xaf, 0xeb, 0x1, 0x9a, 0x5c, 0xfb, 0x8f, 0x5);

// This routine initializes the sensor to its default properties
// Returns an NTSTATUS code
NTSTATUS SdoDevice::Initialize(
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

    m_PoweredOn = FALSE;
    m_FirstSample = TRUE;

    // Initialize the simple device orientation simulator
    HardwareSimulator::Initialize(Device, &m_SimulatorInstance);

    //
    // Create Locks
    //
    {
        Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
        if (!NT_SUCCESS(Status))
        {
            TraceError("SDO %!FUNC! WdfWaitLockCreate m_Lock failed %!STATUS!", Status);
            goto Exit;
        }
    }

    //
    // Create timer object for polling sensor samples
    //
    {
        WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
        TimerAttributes.ParentObject = SensorInstance;
        TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

        WDF_TIMER_CONFIG_INIT(&TimerConfig, OnTimerExpire);
        Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_Timer);
        if (!NT_SUCCESS(Status))
        {
            TraceError("SDO %!FUNC! WdfTimerCreate failed %!STATUS!", Status);
            goto Exit;
        }
    }

    //
    // Sensor Enumeration Properties
    //
    {
        Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ENUMERATION_PROPERTIES_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSORV2_POOL_TAG_SDO,
            Size,
            &MemoryHandle,
            reinterpret_cast<PVOID*>(&m_pEnumerationProperties));
        if (!NT_SUCCESS(Status) || nullptr == m_pEnumerationProperties)
        {
            TraceError("SDO %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
        m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_SimpleDeviceOrientation,
            &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"Microsoft",
            &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"Simple Device Orientation",
            &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_SdoDevice_UniqueID,
            &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));
    }

    //
    // Supported Data-Fields
    //
    {
        Size = SENSOR_PROPERTY_LIST_SIZE(SDO_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSORV2_POOL_TAG_SDO,
            Size,
            &MemoryHandle,
            reinterpret_cast<PVOID*>(&m_pSupportedDataFields));
        if (!NT_SUCCESS(Status) || nullptr == m_pSupportedDataFields)
        {
            TraceError("SDO %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
        m_pSupportedDataFields->Count = SDO_DATA_COUNT;

        m_pSupportedDataFields->List[SDO_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[SDO_DATA_SIMPLEDEVICEORIENTATION] = PKEY_SensorData_SimpleDeviceOrientation;
    }

    //
    // Data
    //
    {
        Size = SENSOR_COLLECTION_LIST_SIZE(SDO_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSORV2_POOL_TAG_SDO,
            Size,
            &MemoryHandle,
            reinterpret_cast<PVOID*>(&m_pData));
        if (!NT_SUCCESS(Status) || nullptr == m_pData)
        {
            TraceError("SDO %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
        m_pData->Count = SDO_DATA_COUNT;

        m_pData->List[SDO_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
        GetSystemTimePreciseAsFileTime(&Time);
        InitPropVariantFromFileTime(&Time, &(m_pData->List[SDO_DATA_TIMESTAMP].Value));

        m_pData->List[SDO_DATA_SIMPLEDEVICEORIENTATION].Key = PKEY_SensorData_SimpleDeviceOrientation;
        InitPropVariantFromUInt32(ABI::Windows::Devices::Sensors::SimpleOrientation::SimpleOrientation_Faceup, &(m_pData->List[SDO_DATA_SIMPLEDEVICEORIENTATION].Value));
    }

    //
    // Sensor Properties
    //
    {
        m_Interval = Sdo_Default_DataInterval;

        Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_PROPERTIES_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSORV2_POOL_TAG_SDO,
            Size,
            &MemoryHandle,
            reinterpret_cast<PVOID*>(&m_pProperties));
        if (!NT_SUCCESS(Status) || nullptr == m_pProperties)
        {
            TraceError("SDO %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
        m_pProperties->Count = SENSOR_PROPERTIES_COUNT;

        m_pProperties->List[SENSOR_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
            &(m_pProperties->List[SENSOR_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(Sdo_Mininum_DataInterval,
            &(m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Value));

        m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
            &(m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_SimpleDeviceOrientation,
            &(m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Value));
    }

    //
    // Empty Threshold List
    //
    {
        Size = SENSOR_COLLECTION_LIST_SIZE(0);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSORV2_POOL_TAG_SDO,
            Size,
            &MemoryHandle,
            reinterpret_cast<PVOID*>(&m_pEmptyThreshold));
        if (!NT_SUCCESS(Status) || nullptr == m_pEmptyThreshold)
        {
            TraceError("SDO %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEmptyThreshold, Size);
        m_pEmptyThreshold->Count = 0;
    }

    // Reset the FirstSample flag
    m_FirstSample = TRUE;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is the AddDevice entry point for the SDO client
// driver. This routine is called by the framework in response to AddDevice
// call from the PnP manager. It will create and initialize the device object
// to represent a new instance of the sensor client.
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnDeviceAdd(
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
        TraceError("SDOS %!FUNC! SensorsCxDeviceInitConfig failed %!STATUS!", Status);
        goto Exit;
    }
    
    // Register the PnP callbacks with the framework.
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&Callbacks);
    Callbacks.EvtDevicePrepareHardware = SdoDevice::OnPrepareHardware;
    Callbacks.EvtDeviceReleaseHardware = SdoDevice::OnReleaseHardware;
    Callbacks.EvtDeviceD0Entry = SdoDevice::OnD0Entry;
    Callbacks.EvtDeviceD0Exit = SdoDevice::OnD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &Callbacks);
    
    // Call the framework to create the device
    Status = WdfDeviceCreate(&pDeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("SDOS %!FUNC! WdfDeviceCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Register CLX callback function pointers
    SENSOR_CONTROLLER_CONFIG_INIT(&SensorConfig);
    SensorConfig.DriverIsPowerPolicyOwner = WdfUseDefault;

    SensorConfig.EvtSensorStart                     = SdoDevice::OnStart;
    SensorConfig.EvtSensorStop                      = SdoDevice::OnStop;
    SensorConfig.EvtSensorGetSupportedDataFields    = SdoDevice::OnGetSupportedDataFields;
    SensorConfig.EvtSensorGetDataInterval           = SdoDevice::OnGetDataInterval;
    SensorConfig.EvtSensorSetDataInterval           = SdoDevice::OnSetDataInterval;
    SensorConfig.EvtSensorGetDataFieldProperties    = SdoDevice::OnGetDataFieldProperties;
    SensorConfig.EvtSensorGetDataThresholds         = SdoDevice::OnGetDataThresholds;
    SensorConfig.EvtSensorSetDataThresholds         = SdoDevice::OnSetDataThresholds;
    SensorConfig.EvtSensorGetProperties             = SdoDevice::OnGetProperties;
    SensorConfig.EvtSensorDeviceIoControl           = SdoDevice::OnIoControl;

    // Set up power capabilities and IO queues
    Status = SensorsCxDeviceInitialize(Device, &SensorConfig);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("SDOS %!FUNC! SensorsCxDeviceInitialize failed %!STATUS!", Status);
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
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnPrepareHardware(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesRaw*/,         // Supplies a handle to a collection of framework resource
                                                // objects. This collection identifies the raw (bus-relative) hardware
                                                // resources that have been assigned to the device.
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
    
{
    PSdoDevice pDevice = nullptr;
    WDF_OBJECT_ATTRIBUTES SensorAttr = {};
    SENSOR_CONFIG SensorConfig = {};
    SENSOROBJECT SensorInstance = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Construct sensor instance

    // Create WDFOBJECT for the sensor
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&SensorAttr, SdoDevice);
    
    // Register sensor instance with clx
    Status = SensorsCxSensorCreate(Device, &SensorAttr, &SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("SDOS %!FUNC! SensorsCxSensorCreate failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("SDOS %!FUNC! GetSdoContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    Status = pDevice->Initialize(Device, SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("SDOS %!FUNC! Initialize device object failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_CONFIG_INIT(&SensorConfig);
    SensorConfig.pEnumerationList = pDevice->m_pEnumerationProperties;
    Status = SensorsCxSensorInitialize(SensorInstance, &SensorConfig);
    if (!NT_SUCCESS(Status))
    {
        TraceError("SDOS %!FUNC! SensorsCxSensorInitialize failed %!STATUS!", Status);
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
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnReleaseHardware(
    _In_ WDFDEVICE Device,                       // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)   // Supplies a handle to a collection of framework
                                                 // resource objects. This collection identifies the translated
                                                 // (system-physical) hardware resources that have been assigned to the
                                                 // device. The resources appear from the CPU's point of view.
{
    PHardwareSimulator pSimulator = nullptr;
    PSdoDevice pDevice = nullptr;
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
        TraceError("SDOS %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! GetSdoContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Delete lock
    if (pDevice->m_Lock)
    {
        WdfObjectDelete(pDevice->m_Lock);
        pDevice->m_Lock = NULL;
    }    
    
    // Cleanup the simple device orientation simulator
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("SDOS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
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
// Returns an NTSTATUS code
NTSTATUS SdoDevice::OnD0Entry(
    _In_ WDFDEVICE Device,                          // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*PreviousState*/)  // WDF_POWER_DEVICE_STATE-typed enumerator that identifies
                                                    // the device power state that the device was in before this transition to D0
{
    PSdoDevice pDevice;
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
        TraceError("SDOS %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! GetSdoContextFromSensorInstance failed %!STATUS!", Status);
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
// Returns an NTSTATUS code
NTSTATUS
SdoDevice::OnD0Exit(
    _In_ WDFDEVICE Device,                       // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*TargetState*/) // Supplies the device power state which the device will be put
                                                 // in once the callback is complete
{
    PSdoDevice pDevice;
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
        TraceError("SDOS %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetSdoContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("SDOS %!FUNC! GetSdoContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Power off sensor
    pDevice->m_PoweredOn = FALSE;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}
