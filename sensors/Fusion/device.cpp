//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of WDF callback functions 
//    for FusionSensor driver.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "HardwareSimulator.h"

#include "Device.tmh"

// FusionSensor Unique ID
// {997335D7-A71C-4C89-9D95-58EAEF917C6A}
//
// TODO: The unique ID below must be set per sensor. A different GUID must be provided for each sensor. Please generate a new GUID.
DEFINE_GUID(GUID_FusionSensorDevice_UniqueID,
    0x997335d7, 0xa71c, 0x4c89, 0x9d, 0x95, 0x58, 0xea, 0xef, 0x91, 0x7c, 0x6a);


// This routine initializes the sensor to its default properties
NTSTATUS
FusionSensorDevice::Initialize(
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

    SENSOR_FunctionEnter();

    // Store device and instance
    m_FxDevice = Device;
    m_SensorInstance = SensorInstance;
    m_Started = FALSE;

    // TODO: Remove the HardwareSimulator code in your final driver. The HardwareSimulator is only used for the purpose of demonstrating how sensor driver samples work.
    // Initialize the FusionSensor simulator
    Status = HardwareSimulator::Initialize(Device, &m_SimulatorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! HardwareSimulator::Initialize failed %!STATUS!", Status);
        goto Exit;
    }

    pSimulator = GetHardwareSimulatorContextFromInstance(m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("FUS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Create Lock
    Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &m_Lock);
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! WdfWaitLockCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Create timer object for polling sensor samples
    WDF_TIMER_CONFIG_INIT(&TimerConfig, FusionSensorDevice::OnTimerExpire);
    WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
    TimerAttributes.ParentObject = SensorInstance;
    TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    Status = WdfTimerCreate(&TimerConfig, &TimerAttributes, &m_Timer);
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! WdfTimerCreate failed %!STATUS!", Status);
        goto Exit;
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
            SENSOR_POOL_TAG_FUSIONSENSOR,
            Size,
            &MemoryHandle,
            (PVOID*)&m_pEnumerationProperties);
        if (!NT_SUCCESS(Status) || m_pEnumerationProperties == nullptr)
        {
            TraceError("FUS %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pEnumerationProperties, Size);
        m_pEnumerationProperties->Count = SENSOR_ENUMERATION_PROPERTIES_COUNT;

        m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Key = DEVPKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_Orientation,
            &(m_pEnumerationProperties->List[SENSOR_TYPE_GUID].Value));

        m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
        InitPropVariantFromString(L"TODO-Set-Manufacturer",
            &(m_pEnumerationProperties->List[SENSOR_MANUFACTURER].Value));

        m_pEnumerationProperties->List[SENSOR_MODEL].Key = DEVPKEY_Sensor_Model;
        InitPropVariantFromString(L"Sample fusion sensor",
            &(m_pEnumerationProperties->List[SENSOR_MODEL].Value));

        m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Key = DEVPKEY_Sensor_PersistentUniqueId;
        InitPropVariantFromCLSID(GUID_FusionSensorDevice_UniqueID,
            &(m_pEnumerationProperties->List[SENSOR_PERSISTENT_UNIQUEID].Value));

        m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Key = DEVPKEY_Sensor_IsPrimary;
        InitPropVariantFromBoolean(FALSE,
            &(m_pEnumerationProperties->List[SENSOR_ISPRIMARY].Value));
    }

    //
    // Supported Data-Fields
    //
    {
        Size = SENSOR_PROPERTY_LIST_SIZE(FUSIONSENSOR_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_FUSIONSENSOR,
            Size,
            &MemoryHandle,
            (PVOID*)&m_pSupportedDataFields);
        if (!NT_SUCCESS(Status) || m_pSupportedDataFields == nullptr)
        {
            TraceError("FUS %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_PROPERTY_LIST_INIT(m_pSupportedDataFields, Size);
        m_pSupportedDataFields->Count = FUSIONSENSOR_DATA_COUNT;

        m_pSupportedDataFields->List[FUSIONSENSOR_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
        m_pSupportedDataFields->List[FUSIONSENSOR_DATA_QUATERNION_W] = PKEY_SensorData_QuaternionW;
        m_pSupportedDataFields->List[FUSIONSENSOR_DATA_QUATERNION_X] = PKEY_SensorData_QuaternionX;
        m_pSupportedDataFields->List[FUSIONSENSOR_DATA_QUATERNION_Y] = PKEY_SensorData_QuaternionY;
        m_pSupportedDataFields->List[FUSIONSENSOR_DATA_QUATERNION_Z] = PKEY_SensorData_QuaternionZ;
        m_pSupportedDataFields->List[FUSIONSENSOR_DATA_ACCURACY] = PKEY_SensorData_MagnetometerAccuracy;
        m_pSupportedDataFields->List[FUSIONSENSOR_DATA_DECLINATION_ANGLE] = PKEY_SensorData_DeclinationAngle_Degrees;
    }

    //
    // Data
    //
    {
        Size = SENSOR_COLLECTION_LIST_SIZE(FUSIONSENSOR_DATA_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_FUSIONSENSOR,
            Size,
            &MemoryHandle,
            (PVOID*)&m_pData);
        if (!NT_SUCCESS(Status) || m_pData == nullptr)
        {
            TraceError("FUS %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pData, Size);
        m_pData->Count = FUSIONSENSOR_DATA_COUNT;

        m_pData->List[FUSIONSENSOR_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
        GetSystemTimePreciseAsFileTime(&Time);
        InitPropVariantFromFileTime(&Time, &(m_pData->List[FUSIONSENSOR_DATA_TIMESTAMP].Value));

        m_pData->List[FUSIONSENSOR_DATA_QUATERNION_W].Key = PKEY_SensorData_QuaternionW;
        InitPropVariantFromFloat(0.0, &(m_pData->List[FUSIONSENSOR_DATA_QUATERNION_W].Value));

        m_pData->List[FUSIONSENSOR_DATA_QUATERNION_X].Key = PKEY_SensorData_QuaternionX;
        InitPropVariantFromFloat(0.0, &(m_pData->List[FUSIONSENSOR_DATA_QUATERNION_X].Value));

        m_pData->List[FUSIONSENSOR_DATA_QUATERNION_Y].Key = PKEY_SensorData_QuaternionY;
        InitPropVariantFromFloat(0.0, &(m_pData->List[FUSIONSENSOR_DATA_QUATERNION_Y].Value));

        m_pData->List[FUSIONSENSOR_DATA_QUATERNION_Z].Key = PKEY_SensorData_QuaternionZ;
        InitPropVariantFromFloat(0.0, &(m_pData->List[FUSIONSENSOR_DATA_QUATERNION_Z].Value));

        m_pData->List[FUSIONSENSOR_DATA_ACCURACY].Key = PKEY_SensorData_MagnetometerAccuracy;
        InitPropVariantFromUInt32(Unknown, &(m_pData->List[FUSIONSENSOR_DATA_ACCURACY].Value));

        // Declination angle is optional, it will be computed by the system if not present.
        m_pData->List[FUSIONSENSOR_DATA_DECLINATION_ANGLE].Key = PKEY_SensorData_DeclinationAngle_Degrees;
        InitPropVariantFromFloat(0.0, &(m_pData->List[FUSIONSENSOR_DATA_DECLINATION_ANGLE].Value));
    }

    //
    // Sensor Properties
    //
    {
        m_Interval = FusionSensor_Default_DataInterval;

        Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_PROPERTIES_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_FUSIONSENSOR,
            Size,
            &MemoryHandle,
            (PVOID*)&m_pProperties);
        if (!NT_SUCCESS(Status) || m_pProperties == nullptr)
        {
            TraceError("FUS %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pProperties, Size);
        m_pProperties->Count = SENSOR_PROPERTIES_COUNT;

        m_pProperties->List[SENSOR_PROPERTY_STATE].Key = PKEY_Sensor_State;
        InitPropVariantFromUInt32(SensorState_Initializing,
            &(m_pProperties->List[SENSOR_PROPERTY_STATE].Value));

        m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
        InitPropVariantFromUInt32(FusionSensor_Default_MinDataInterval_Ms,
            &(m_pProperties->List[SENSOR_PROPERTY_MIN_INTERVAL].Value));

        m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
        InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pData),
            &(m_pProperties->List[SENSOR_PROPERTY_MAX_DATAFIELDSIZE].Value));

        m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Key = PKEY_Sensor_Type;
        InitPropVariantFromCLSID(GUID_SensorType_Orientation,
            &(m_pProperties->List[SENSOR_PROPERTY_SENSOR_TYPE].Value));

        m_pProperties->List[SENSOR_PROPERTY_USE_GYRO].Key = PKEY_OrientationSensor_GyroscopeUsed;
        InitPropVariantFromBoolean(FALSE, &(m_pProperties->List[SENSOR_PROPERTY_USE_GYRO].Value));
    }

    //
    // Accelerometer related data field properties
    //
    {
        Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_ACC_DATA_FIELD_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_FUSIONSENSOR,
            Size,
            &MemoryHandle,
            (PVOID*)&m_pAccDataFieldProperties);
        if (!NT_SUCCESS(Status) || m_pAccDataFieldProperties == nullptr)
        {
            TraceError("FUS %!FUNC! Fusion sensor WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pAccDataFieldProperties, Size);
        m_pAccDataFieldProperties->Count = SENSOR_ACC_DATA_FIELD_PROPERTY_COUNT;

        m_pAccDataFieldProperties->List[SENSOR_ACC_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat((float)AccFakeDevice_Axis_Resolution,
            &(m_pAccDataFieldProperties->List[SENSOR_ACC_RESOLUTION].Value));

        m_pAccDataFieldProperties->List[SENSOR_ACC_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(AccFakeDevice_Axis_Minimum,
            &(m_pAccDataFieldProperties->List[SENSOR_ACC_MIN_RANGE].Value));

        m_pAccDataFieldProperties->List[SENSOR_ACC_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(AccFakeDevice_Axis_Maximum,
            &(m_pAccDataFieldProperties->List[SENSOR_ACC_MAX_RANGE].Value));

    }

    //
    // Gyroscope related data field properties
    //
    {
        Size = SENSOR_COLLECTION_LIST_SIZE(SENSOR_GYR_DATA_FIELD_PROPERTY_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_FUSIONSENSOR,
            Size,
            &MemoryHandle,
            (PVOID*)&m_pGyrDataFieldProperties);
        if (!NT_SUCCESS(Status) || m_pGyrDataFieldProperties == nullptr)
        {
            TraceError("FUS %!FUNC! Fusion sensor WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pGyrDataFieldProperties, Size);
        m_pGyrDataFieldProperties->Count = SENSOR_GYR_DATA_FIELD_PROPERTY_COUNT;

        m_pGyrDataFieldProperties->List[SENSOR_GYR_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
        InitPropVariantFromFloat(GyrFakeDevice_Resolution_DegreesPerSecond,
            &(m_pGyrDataFieldProperties->List[SENSOR_GYR_RESOLUTION].Value));

        m_pGyrDataFieldProperties->List[SENSOR_GYR_MIN_RANGE].Key = PKEY_SensorDataField_RangeMinimum;
        InitPropVariantFromFloat(GyrFakeDevice_Minimum_DegreesPerSecond,
            &(m_pGyrDataFieldProperties->List[SENSOR_GYR_MIN_RANGE].Value));

        m_pGyrDataFieldProperties->List[SENSOR_GYR_MAX_RANGE].Key = PKEY_SensorDataField_RangeMaximum;
        InitPropVariantFromFloat(GyrFakeDevice_Maximum_DegreesPerSecond,
            &(m_pGyrDataFieldProperties->List[SENSOR_GYR_MAX_RANGE].Value));
    }

    //
    // Set default threshold
    //
    {
        Size = SENSOR_COLLECTION_LIST_SIZE(FUSIONSENSOR_THRESHOLD_COUNT);

        MemoryHandle = NULL;
        WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
        MemoryAttributes.ParentObject = SensorInstance;
        Status = WdfMemoryCreate(&MemoryAttributes,
            PagedPool,
            SENSOR_POOL_TAG_FUSIONSENSOR,
            Size,
            &MemoryHandle,
            (PVOID*)&m_pThresholds);
        if (!NT_SUCCESS(Status) || m_pThresholds == nullptr)
        {
            TraceError("FUS %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
            goto Exit;
        }

        SENSOR_COLLECTION_LIST_INIT(m_pThresholds, Size);
        m_pThresholds->Count = FUSIONSENSOR_THRESHOLD_COUNT;

        m_pThresholds->List[FUSIONSENSOR_THRESHOLD_ROTATION_ANGLE].Key = PKEY_SensorData_RotationAngle_Degrees;
        InitPropVariantFromFloat(0.0, &(m_pThresholds->List[FUSIONSENSOR_THRESHOLD_ROTATION_ANGLE].Value));

        m_pThresholds->List[FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_X].Key = PKEY_SensorData_LinearAccelerationX_Gs;
        InitPropVariantFromFloat(0.0, &(m_pThresholds->List[FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_X].Value));

        m_pThresholds->List[FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_Y].Key = PKEY_SensorData_LinearAccelerationY_Gs;
        InitPropVariantFromFloat(0.0, &(m_pThresholds->List[FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_Y].Value));

        m_pThresholds->List[FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_Z].Key = PKEY_SensorData_LinearAccelerationZ_Gs;
        InitPropVariantFromFloat(0.0, &(m_pThresholds->List[FUSIONSENSOR_THRESHOLD_LINEAR_ACCELERATION_Z].Value));

        m_pThresholds->List[FUSIONSENSOR_THRESHOLD_ROTATION_RATE_X].Key = PKEY_SensorData_CorrectedAngularVelocityX_DegreesPerSecond;
        InitPropVariantFromFloat(0.0, &(m_pThresholds->List[FUSIONSENSOR_THRESHOLD_ROTATION_RATE_X].Value));

        m_pThresholds->List[FUSIONSENSOR_THRESHOLD_ROTATION_RATE_Y].Key = PKEY_SensorData_CorrectedAngularVelocityY_DegreesPerSecond;
        InitPropVariantFromFloat(0.0, &(m_pThresholds->List[FUSIONSENSOR_THRESHOLD_ROTATION_RATE_Y].Value));

        m_pThresholds->List[FUSIONSENSOR_THRESHOLD_ROTATION_RATE_Z].Key = PKEY_SensorData_CorrectedAngularVelocityZ_DegreesPerSecond;
        InitPropVariantFromFloat(0.0, &(m_pThresholds->List[FUSIONSENSOR_THRESHOLD_ROTATION_RATE_Z].Value));

        ZeroMemory(&m_CachedThresholds, sizeof(m_CachedThresholds));
    }

    ZeroMemory(&m_LastSample, sizeof(m_LastSample));

    // Set default threshold
    m_FirstSample = TRUE;

Exit:
    SENSOR_FunctionExit(Status);
    return Status;
}



// This routine is the AddDevice entry point for the FusionSensor client
// driver. This routine is called by the framework in response to AddDevice
// call from the PnP manager. It will create and initialize the device object
// to represent a new instance of the sensor client.
NTSTATUS
FusionSensorDevice::OnDeviceAdd(
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
        TraceError("FUS %!FUNC! SensorsCxDeviceInitConfig failed %!STATUS!", Status);
        goto Exit;
    }
    
    // Register the PnP callbacks with the framework.
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&Callbacks);
    Callbacks.EvtDevicePrepareHardware = FusionSensorDevice::OnPrepareHardware;
    Callbacks.EvtDeviceReleaseHardware = FusionSensorDevice::OnReleaseHardware;
    Callbacks.EvtDeviceD0Entry = FusionSensorDevice::OnD0Entry;
    Callbacks.EvtDeviceD0Exit = FusionSensorDevice::OnD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &Callbacks);
    
    // Call the framework to create the device
    Status = WdfDeviceCreate(&pDeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("FUS %!FUNC! WdfDeviceCreate failed %!STATUS!", Status);
        goto Exit;
    }

    // Register CLX callback function pointers
    SENSOR_CONTROLLER_CONFIG_INIT(&SensorConfig);
    SensorConfig.DriverIsPowerPolicyOwner = WdfUseDefault;

    SensorConfig.EvtSensorStart                     = FusionSensorDevice::OnStart;
    SensorConfig.EvtSensorStop                      = FusionSensorDevice::OnStop;
    SensorConfig.EvtSensorGetSupportedDataFields    = FusionSensorDevice::OnGetSupportedDataFields;
    SensorConfig.EvtSensorGetDataInterval           = FusionSensorDevice::OnGetDataInterval;
    SensorConfig.EvtSensorSetDataInterval           = FusionSensorDevice::OnSetDataInterval;
    SensorConfig.EvtSensorGetDataFieldProperties    = FusionSensorDevice::OnGetDataFieldProperties;
    SensorConfig.EvtSensorGetDataThresholds         = FusionSensorDevice::OnGetDataThresholds;
    SensorConfig.EvtSensorSetDataThresholds         = FusionSensorDevice::OnSetDataThresholds;
    SensorConfig.EvtSensorGetProperties             = FusionSensorDevice::OnGetProperties;
    SensorConfig.EvtSensorDeviceIoControl           = FusionSensorDevice::OnIoControl;

    // Set up power capabilities and IO queues
    Status = SensorsCxDeviceInitialize(Device, &SensorConfig);
    if (!NT_SUCCESS(Status)) 
    {
        TraceError("FUS %!FUNC! SensorsCxDeviceInitialize failed %!STATUS!", Status);
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
FusionSensorDevice::OnPrepareHardware(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesRaw*/,         // Supplies a handle to a collection of framework resource
                                                // objects. This collection identifies the raw (bus-relative) hardware
                                                // resources that have been assigned to the device.
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
{
    PFusionSensorDevice pDevice = nullptr;
    WDF_OBJECT_ATTRIBUTES SensorAttr = {};
    SENSOR_CONFIG SensorConfig = {};
    SENSOROBJECT SensorInstance = nullptr;
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Construct sensor instance

    // Create WDFOBJECT for the sensor
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&SensorAttr, FusionSensorDevice);
    
    // Register sensor instance with clx

    Status = SensorsCxSensorCreate(Device, &SensorAttr, &SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! SensorsCxSensorCreate failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("FUS %!FUNC! GetFusionSensorContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // Device initialization

    Status = pDevice->Initialize(Device, SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! Initialize device object failed %!STATUS!", Status);
        goto Exit;
    }

    SENSOR_CONFIG_INIT(&SensorConfig);
    SensorConfig.pEnumerationList = pDevice->m_pEnumerationProperties;
    Status = SensorsCxSensorInitialize(SensorInstance, &SensorConfig);
    if (!NT_SUCCESS(Status))
    {
        TraceError("FUS %!FUNC! SensorsCxSensorInitialize failed %!STATUS!", Status);
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
FusionSensorDevice::OnReleaseHardware(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
{
    PHardwareSimulator pSimulator = nullptr;
    PFusionSensorDevice pDevice = nullptr;
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
        TraceError("FUS %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! GetFusionSensorContextFromSensorInstance failed %!STATUS!", Status);
        goto Exit;
    }

    // TODO: Remove the HardwareSimulator code in your final driver. The HardwareSimulator is only used for the purpose of demonstrating how sensor driver samples work.
    pSimulator = GetHardwareSimulatorContextFromInstance(pDevice->m_SimulatorInstance);
    if (nullptr == pSimulator)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceError("FUS %!FUNC! GetHardwareSimulatorContextFromInstance failed %!STATUS!", Status);
        goto Exit;
    }


    // Delete lock
    if (NULL != pDevice->m_Lock)
    {
        WdfObjectDelete(pDevice->m_Lock);
        pDevice->m_Lock = NULL;
    }    
    
    // Cleanup the FusionSensor simulator
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
FusionSensorDevice::OnD0Entry(
    _In_ WDFDEVICE Device,                          // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*PreviousState*/)  // WDF_POWER_DEVICE_STATE-typed enumerator that identifies
                                                    // the device power state that the device was in before this transition to D0
{
    PFusionSensorDevice pDevice;
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
        TraceError("FUS %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! GetFusionSensorContextFromSensorInstance failed %!STATUS!", Status);
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
FusionSensorDevice::OnD0Exit(
    _In_ WDFDEVICE Device,                       // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE /*TargetState*/) // Supplies the device power state which the device will be put
                                                 // in once the callback is complete
{
    PFusionSensorDevice pDevice;
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
        TraceError("FUS %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
        goto Exit;
    }

    pDevice = GetFusionSensorContextFromSensorInstance(SensorInstance);
    if (nullptr == pDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("FUS %!FUNC! GetFusionSensorContextFromSensorInstance failed %!STATUS!", Status);
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
