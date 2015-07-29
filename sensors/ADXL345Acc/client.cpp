//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of driver callback functions
//    from clx to ADXL345 accelerometer.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "Adxl345.h"

#include <timeapi.h>

#include "Client.tmh"


// Analog Adxl345 Unique ID
// {EF2C014C-DEBA-43F4-890D-978095684DD6}
DEFINE_GUID(GUID_Adxl345Device_UniqueID,
    0xef2c014c, 0xdeba, 0x43f4, 0x89, 0xd, 0x97, 0x80, 0x95, 0x68, 0x4d, 0xd6);


// Helper function for initializing ADXL345AccDevice. Returns status.
inline NTSTATUS InitSensorCollection(
    _In_ ULONG CollectionListCount,
    _Outptr_ PSENSOR_COLLECTION_LIST *CollectionList,
    _In_ SENSOROBJECT SensorInstance)                   // SENSOROBJECT for sensor instance
{
    WDF_OBJECT_ATTRIBUTES MemoryAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;

    WDFMEMORY MemoryHandle = NULL;
    ULONG MemorySize = SENSOR_COLLECTION_LIST_SIZE(CollectionListCount);
    NTSTATUS Status = WdfMemoryCreate(&MemoryAttributes,
                                      PagedPool,
                                      SENSORV2_POOL_TAG_ACCELEROMETER,
                                      MemorySize,
                                      &MemoryHandle,
                                      reinterpret_cast<PVOID*>(CollectionList));
    if (!NT_SUCCESS(Status) || nullptr == *CollectionList)
    {
        Status = STATUS_UNSUCCESSFUL;
        TraceError("ACC %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        return Status;
    }

    SENSOR_COLLECTION_LIST_INIT(*CollectionList, MemorySize);
    (*CollectionList)->Count = CollectionListCount;
    return Status;
}

// Helper function for initializing ADXL345AccDevice. Returns status.
inline NTSTATUS InitSensorProperty(
    _In_ ULONG PropertyListCount,
    _Outptr_ PSENSOR_PROPERTY_LIST *PropertyList,
    _In_ SENSOROBJECT SensorInstance)               // SENSOROBJECT for sensor instance
{
    WDF_OBJECT_ATTRIBUTES MemoryAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;

    WDFMEMORY MemoryHandle = NULL;
    ULONG MemorySize = SENSOR_PROPERTY_LIST_SIZE(PropertyListCount);
    NTSTATUS Status = WdfMemoryCreate(&MemoryAttributes,
                                      PagedPool,
                                      SENSORV2_POOL_TAG_ACCELEROMETER,
                                      MemorySize,
                                      &MemoryHandle,
                                      reinterpret_cast<PVOID*>(PropertyList));
    if (!NT_SUCCESS(Status) || nullptr == *PropertyList)
    {
        Status = STATUS_UNSUCCESSFUL;
        TraceError("ACC %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        return Status;
    }

    SENSOR_PROPERTY_LIST_INIT(*PropertyList, MemorySize);
    (*PropertyList)->Count = PropertyListCount;
    return Status;
}

// This routine initializes the sensor to its default properties
NTSTATUS ADXL345AccDevice::Initialize(
    _In_ WDFDEVICE Device,            // WDFDEVICE object
    _In_ SENSOROBJECT SensorInstance) // SENSOROBJECT for each sensor instance
{
    SENSOR_FunctionEnter();

    // Store device and instance
    m_Device = Device;
    m_SensorInstance = SensorInstance;
    m_Started = false;

    // Create Lock
    NTSTATUS Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &(m_I2CWaitLock));
    if (!NT_SUCCESS(Status))
    {
        TraceError("ACC %!FUNC! WdfWaitLockCreate failed %!STATUS!", Status);
    }

    // Sensor Enumeration Properties
    if (NT_SUCCESS(Status))
    {
        Status = InitSensorCollection(SENSOR_ENUMERATION_PROPERTIES_COUNT, &m_pEnumerationProperties, SensorInstance);
        if (NT_SUCCESS(Status))
        {
            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_TYPE].Key = DEVPKEY_Sensor_Type;
            InitPropVariantFromCLSID(GUID_SensorType_Accelerometer3D,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_TYPE].Value));
        
            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
            InitPropVariantFromString(SENSOR_ACCELEROMETER_MANUFACTURER,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_MANUFACTURER].Value));
        
            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_MODEL].Key = DEVPKEY_Sensor_Model;
            InitPropVariantFromString(SENSOR_ACCELEROMETER_MODEL,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_MODEL].Value));
            
            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_CONNECTION_TYPE].Key = DEVPKEY_Sensor_ConnectionType;
            // The DEVPKEY_Sensor_ConnectionType values match the SensorConnectionType enumeration
            InitPropVariantFromUInt32(0, // 0: INTEGRATED, 1: ATTACHED, 2: EXTERNAL
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_CONNECTION_TYPE].Value));

            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_PERSISTENT_UNIQUE_ID].Key = DEVPKEY_Sensor_PersistentUniqueId;
            InitPropVariantFromCLSID(GUID_Adxl345Device_UniqueID,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_PERSISTENT_UNIQUE_ID].Value));

            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_CATEGORY].Key = DEVPKEY_Sensor_Category;
            InitPropVariantFromCLSID(GUID_SensorCategory_Motion,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_CATEGORY].Value));
        }
    }

    // Supported Data-Fields
    if (NT_SUCCESS(Status))
    {
        Status = InitSensorProperty(SENSOR_DATA_COUNT, &m_pSupportedDataFields, SensorInstance);
        if (NT_SUCCESS(Status))
        {
            m_pSupportedDataFields->List[SENSOR_DATA_TIMESTAMP] = PKEY_SensorData_Timestamp;
            m_pSupportedDataFields->List[SENSOR_DATA_ACCELERATION_X_G] = PKEY_SensorData_AccelerationX_Gs;
            m_pSupportedDataFields->List[SENSOR_DATA_ACCELERATION_Y_G] = PKEY_SensorData_AccelerationY_Gs;
            m_pSupportedDataFields->List[SENSOR_DATA_ACCELERATION_Z_G] = PKEY_SensorData_AccelerationZ_Gs;
        }
    }

    // Sensor Data
    if (NT_SUCCESS(Status))
    {
        Status = InitSensorCollection(SENSOR_DATA_COUNT, &m_pSensorData, SensorInstance);
        if (NT_SUCCESS(Status))
        {
            FILETIME time;
            m_pSensorData->List[SENSOR_DATA_TIMESTAMP].Key = PKEY_SensorData_Timestamp;
            GetSystemTimePreciseAsFileTime(&time);
            InitPropVariantFromFileTime(&time, &(m_pSensorData->List[SENSOR_DATA_TIMESTAMP].Value));
        
            m_pSensorData->List[SENSOR_DATA_ACCELERATION_X_G].Key = PKEY_SensorData_AccelerationX_Gs;
            InitPropVariantFromFloat(0.0f, &(m_pSensorData->List[SENSOR_DATA_ACCELERATION_X_G].Value));
        
            m_pSensorData->List[SENSOR_DATA_ACCELERATION_Y_G].Key = PKEY_SensorData_AccelerationY_Gs;
            InitPropVariantFromFloat(0.0f, &(m_pSensorData->List[SENSOR_DATA_ACCELERATION_Y_G].Value));
        
            m_pSensorData->List[SENSOR_DATA_ACCELERATION_Z_G].Key = PKEY_SensorData_AccelerationZ_Gs;
            InitPropVariantFromFloat(0.0f, &(m_pSensorData->List[SENSOR_DATA_ACCELERATION_Z_G].Value));
        }
    }

    // Sensor Properties
    if (NT_SUCCESS(Status))
    {
        Status = InitSensorCollection(SENSOR_PROPERTIES_COUNT, &m_pSensorProperties, SensorInstance);
        if (NT_SUCCESS(Status))
        {
            m_Interval = DEFAULT_ACCELEROMETER_REPORT_INTERVAL;
    
            m_pSensorProperties->List[SENSOR_PROPERTY_STATE].Key = PKEY_Sensor_State;
            InitPropVariantFromUInt32(SensorState_Initializing, &(m_pSensorProperties->List[SENSOR_PROPERTY_STATE].Value));
        
            m_pSensorProperties->List[SENSOR_PROPERTY_MIN_DATA_INTERVAL].Key = PKEY_Sensor_MinimumDataInterval_Ms;
            InitPropVariantFromUInt32(ACCELEROMETER_MIN_REPORT_INTERVAL, &(m_pSensorProperties->List[SENSOR_PROPERTY_MIN_DATA_INTERVAL].Value));
        
            m_pSensorProperties->List[SENSOR_PROPERTY_MAX_DATA_FIELD_SIZE].Key = PKEY_Sensor_MaximumDataFieldSize_Bytes;
            InitPropVariantFromUInt32(CollectionsListGetMarshalledSize(m_pSensorData), &(m_pSensorProperties->List[SENSOR_PROPERTY_MAX_DATA_FIELD_SIZE].Value));
        
            m_pSensorProperties->List[SENSOR_PROPERTY_TYPE].Key = PKEY_Sensor_Type;
            InitPropVariantFromCLSID(GUID_SensorType_Accelerometer3D, &(m_pSensorProperties->List[SENSOR_PROPERTY_TYPE].Value)); 
        }
    }

    // Data field properties
    if (NT_SUCCESS(Status))
    {
        Status = InitSensorCollection(SENSOR_DATA_FIELD_PROPERTIES_COUNT, &m_pDataFieldProperties, SensorInstance);
        if (NT_SUCCESS(Status))
        {
            m_pDataFieldProperties->List[SENSOR_DATA_FIELD_PROPERTY_RESOLUTION].Key = PKEY_SensorDataField_Resolution;
            InitPropVariantFromFloat(static_cast<float>(ACCELEROMETER_CHANGE_SENSITIVITY_RESOLUTION), &(m_pDataFieldProperties->List[SENSOR_DATA_FIELD_PROPERTY_RESOLUTION].Value));
        
            m_pDataFieldProperties->List[SENSOR_DATA_FIELD_PROPERTY_RANGE_MIN].Key = PKEY_SensorDataField_RangeMinimum;
            InitPropVariantFromFloat(static_cast<float>(ACCELEROMETER_MIN_CHANGE_SENSITIVITY), &(m_pDataFieldProperties->List[SENSOR_DATA_FIELD_PROPERTY_RANGE_MIN].Value));
        
            m_pDataFieldProperties->List[SENSOR_DATA_FIELD_PROPERTY_RANGE_MAX].Key = PKEY_SensorDataField_RangeMaximum;
            InitPropVariantFromFloat(static_cast<float>(ACCELEROMETER_MAX_CHANGE_SENSITIVITY), &(m_pDataFieldProperties->List[SENSOR_DATA_FIELD_PROPERTY_RANGE_MAX].Value));
        }
    }

    // Set default threshold
    if (NT_SUCCESS(Status))
    {
        // note: COUNT-1 as timestamp does not have thresholds
        Status = InitSensorCollection(SENSOR_DATA_COUNT-1, &m_pThresholds, SensorInstance);
        if NT_SUCCESS(Status)
        {
            m_CachedThresholds.X = static_cast<float>(ACCELEROMETER_DEFAULT_AXIS_THRESHOLD);
            m_CachedThresholds.Y = static_cast<float>(ACCELEROMETER_DEFAULT_AXIS_THRESHOLD);
            m_CachedThresholds.Z = static_cast<float>(ACCELEROMETER_DEFAULT_AXIS_THRESHOLD);
            
            m_pThresholds->List[SENSOR_DATA_ACCELERATION_X_G].Key = PKEY_SensorData_AccelerationX_Gs;
            InitPropVariantFromFloat(static_cast<float>(ACCELEROMETER_DEFAULT_AXIS_THRESHOLD), &(m_pThresholds->List[SENSOR_DATA_ACCELERATION_X_G].Value));
        
            m_pThresholds->List[SENSOR_DATA_ACCELERATION_Y_G].Key = PKEY_SensorData_AccelerationY_Gs;
            InitPropVariantFromFloat(static_cast<float>(ACCELEROMETER_DEFAULT_AXIS_THRESHOLD), &(m_pThresholds->List[SENSOR_DATA_ACCELERATION_Y_G].Value));
        
            m_pThresholds->List[SENSOR_DATA_ACCELERATION_Z_G].Key = PKEY_SensorData_AccelerationZ_Gs;
            InitPropVariantFromFloat(static_cast<float>(ACCELEROMETER_DEFAULT_AXIS_THRESHOLD), &(m_pThresholds->List[SENSOR_DATA_ACCELERATION_Z_G].Value));
        }
    }

    // Reset the FirstSample flag
    if (NT_SUCCESS(Status))
    {
        m_FirstSample = true;
    }
    // Trace to this function in case of failure
    else
    {
        TraceError("ACC %!FUNC! failed %!STATUS!", Status);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

VOID ADXL345AccDevice::DeInit()
{
    // Delete lock
    if (NULL != m_I2CWaitLock)
    {
        WdfObjectDelete(m_I2CWaitLock);
        m_I2CWaitLock = NULL;
    }

    // Delete sensor instance
    if (NULL != m_SensorInstance)
    {
        WdfObjectDelete(m_SensorInstance);
    }
}

// This routine reads a single sample, compares threshold and pushes sample
// to sensor class extension. This routine is protected by the caller.
NTSTATUS ADXL345AccDevice::GetData() 
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Read the device data
    BYTE DataBuffer[ADXL345_DATA_REPORT_SIZE_BYTES];
    WdfWaitLockAcquire(m_I2CWaitLock, NULL);
    Status = I2CSensorReadRegister(m_I2CIoTarget, ADXL345_DATA_X0, &DataBuffer[0], sizeof(DataBuffer));
    WdfWaitLockRelease(m_I2CWaitLock);
    if (!NT_SUCCESS(Status))
    {
        TraceError("ACC %!FUNC! I2CSensorReadRegister from 0x%02x failed! %!STATUS!", ADXL345_DATA_X0, Status);
    }
    else
    {
        bool DataReady = false;

        // Perform data conversion
        SHORT xRaw = static_cast<SHORT>((DataBuffer[1] << 8) | DataBuffer[0]);
        SHORT yRaw = static_cast<SHORT>((DataBuffer[3] << 8) | DataBuffer[2]);
        SHORT zRaw = static_cast<SHORT>((DataBuffer[5] << 8) | DataBuffer[4]);
    
        const float ScaleFactor = 1 / 256.0F;
        VEC3D Sample = {};
        Sample.X = static_cast<float>(xRaw * ScaleFactor);
        Sample.Y = static_cast<float>(yRaw * ScaleFactor);
        Sample.Z = static_cast<float>(zRaw * ScaleFactor);
    
        // Set data ready if this is the first sample or we have exceeded the thresholds
        if (m_FirstSample)
        {
            m_FirstSample = false;
            DataReady = true;
        }
        else if ((fabsf(Sample.X - m_LastSample.X) >= m_CachedThresholds.X) ||
                 (fabsf(Sample.Y - m_LastSample.Y) >= m_CachedThresholds.Y) ||
                 (fabsf(Sample.Z - m_LastSample.Z) >= m_CachedThresholds.Z))
        {
            DataReady = true;
        }
    
        if (DataReady) 
        {
            // Update values for SW thresholding and send data to class extension
            m_LastSample.X = Sample.X;
            m_LastSample.Y = Sample.Y;
            m_LastSample.Z = Sample.Z;
    
            // Save the data in the context
            InitPropVariantFromFloat(Sample.X, &(m_pSensorData->List[SENSOR_DATA_ACCELERATION_X_G].Value));
            InitPropVariantFromFloat(Sample.Y, &(m_pSensorData->List[SENSOR_DATA_ACCELERATION_Y_G].Value));
            InitPropVariantFromFloat(Sample.Z, &(m_pSensorData->List[SENSOR_DATA_ACCELERATION_Z_G].Value));
    
            FILETIME Timestamp = {};
            GetSystemTimePreciseAsFileTime(&Timestamp);
            InitPropVariantFromFileTime(&Timestamp, &(m_pSensorData->List[SENSOR_DATA_TIMESTAMP].Value));
    
            SensorsCxSensorDataReady(m_SensorInstance, m_pSensorData);
        }
        else 
        {
            TraceInformation("ACC %!FUNC! Data did NOT meet the threshold");
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Services a hardware interrupt.
BOOLEAN ADXL345AccDevice::OnInterruptIsr(
    _In_ WDFINTERRUPT Interrupt,        // Handle to a framework interrupt object
    _In_ ULONG /*MessageID*/)           // If the device is using message-signaled interrupts (MSIs),
                                        // this parameter is the message number that identifies the
                                        // device's hardware interrupt message. Otherwise, this value is 0.
{
    BOOLEAN InterruptRecognized = FALSE;
    PADXL345AccDevice pAccDevice = nullptr;

    SENSOR_FunctionEnter();

    // Get the sensor instance
    ULONG SensorInstanceCount = 1;
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxDeviceGetSensorList(WdfInterruptGetDevice(Interrupt), &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) || 0 == SensorInstanceCount || NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
    }

    // Get the device context
    else // if (NT_SUCCESS(Status))
    {
        pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! GetADXL345AccContextFromSensorInstance failed %!STATUS!", Status);
        }
    }

    // Read the interrupt source
    if (NT_SUCCESS(Status))
    {
        BYTE IntSrcBuffer = 0;
        WdfWaitLockAcquire(pAccDevice->m_I2CWaitLock, NULL);
        Status = I2CSensorReadRegister(pAccDevice->m_I2CIoTarget, ADXL345_INT_SOURCE, &IntSrcBuffer, sizeof(IntSrcBuffer));
        WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
    
        if (!NT_SUCCESS(Status)) 
        {
            TraceError("ACC %!FUNC! I2CSensorReadRegister from 0x%02x failed! %!STATUS!", ADXL345_INT_SOURCE, Status);
        }
        else if ((IntSrcBuffer & ADXL345_INT_ACTIVITY) == 0)
        {      
            TraceError("%!FUNC! Interrupt source not recognized");
        }
        else
        {
            InterruptRecognized = TRUE;
            BOOLEAN WorkItemQueued = WdfInterruptQueueWorkItemForIsr(Interrupt);
            TraceVerbose("%!FUNC! Work item %s queued for interrupt", WorkItemQueued ? "" : " already");
        }
    }

    SENSOR_FunctionExit(Status);
    return InterruptRecognized;
}

// Processes interrupt information that the driver's EvtInterruptIsr callback function has stored.
VOID ADXL345AccDevice::OnInterruptWorkItem(
    _In_ WDFINTERRUPT Interrupt,            // Handle to a framework object
    _In_ WDFOBJECT /*AssociatedObject*/)    // A handle to the framework device object that 
                                            // the driver passed to WdfInterruptCreate.
{
    PADXL345AccDevice pAccDevice = nullptr;

    SENSOR_FunctionEnter();

    // Get the sensor instance
    ULONG SensorInstanceCount = 1;
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxDeviceGetSensorList(WdfInterruptGetDevice(Interrupt), &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) || 0 == SensorInstanceCount || NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
    }

    // Get the device context
    else //if (NT_SUCCESS(Status))
    {
        pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice) 
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! GetADXL345AccContextFromSensorInstance failed %!STATUS!", Status);
        }
    }

    // Read the device data
    if (NT_SUCCESS(Status))
    {
        WdfInterruptAcquireLock(Interrupt);
        Status = pAccDevice->GetData();
        WdfInterruptReleaseLock(Interrupt);
        if (!NT_SUCCESS(Status) && STATUS_DATA_NOT_ACCEPTED != Status) 
        {
            TraceError("ACC %!FUNC! GetData failed %!STATUS!", Status);
        }
    }

    SENSOR_FunctionExit(Status);
}

// Called by Sensor CLX to begin continously sampling the sensor.
NTSTATUS ADXL345AccDevice::OnStart(
    _In_ SENSOROBJECT SensorInstance)    // Sensor device object
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Get the device context
    PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
    if (nullptr == pAccDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! Sensor(%08X) parameter is invalid %!STATUS!", (INT) SensorInstance, Status);
    }
    else if (!pAccDevice->m_PoweredOn)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! Sensor is not powered on! %!STATUS!", Status);
    }
    else
    {
        WdfWaitLockAcquire(pAccDevice->m_I2CWaitLock, NULL);
    
        // Set accelerometer to measurement mode
        REGISTER_SETTING RegisterSetting = { ADXL345_POWER_CTL, ADXL345_POWER_CTL_MEASURE };
        Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
        if (!NT_SUCCESS(Status))
        {
            WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
            TraceError("ACC %!FUNC! I2CSensorWriteRegister to 0x%02x failed! %!STATUS!", RegisterSetting.Register, Status);
        }

        // Enable interrupts
        else // if (NT_SUCCESS(Status))
        {
            RegisterSetting = { ADXL345_INT_ENABLE, ADXL345_INT_ACTIVITY };
            Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
            WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
            if (!NT_SUCCESS(Status))
            {
                TraceError("ACC %!FUNC! I2CSensorWriteRegister to 0x%02x failed! %!STATUS!", RegisterSetting.Register, Status);
            }
        }

        if (NT_SUCCESS(Status))
        {
            pAccDevice->m_FirstSample = true;
            pAccDevice->m_Started = true;
            
            InitPropVariantFromUInt32(SensorState_Active, &(pAccDevice->m_pSensorProperties->List[SENSOR_PROPERTY_STATE].Value));
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to stop continously sampling the sensor.
NTSTATUS ADXL345AccDevice::OnStop(
    _In_ SENSOROBJECT SensorInstance)   // Sensor device object
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Get the device context
    PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
    if (nullptr == pAccDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! Sensor(%08X) parameter is invalid %!STATUS!", (INT)SensorInstance, Status);
    }
    else
    {
        pAccDevice->m_Started = false;
    
        // Disable interrupts
        REGISTER_SETTING RegisterSetting = { ADXL345_INT_ENABLE, 0 };
        WdfWaitLockAcquire(pAccDevice->m_I2CWaitLock, NULL);
        Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! I2CSensorWriteRegister to 0x%02x failed! %!STATUS!", RegisterSetting.Register, Status);
        }
        // Clear any stale interrupts
        else
        {
            RegisterSetting = { ADXL345_INT_SOURCE, 0 };
            Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
            if (!NT_SUCCESS(Status))
            {
                TraceError("ACC %!FUNC! I2CSensorReadRegister from 0x%02x failed! %!STATUS!", RegisterSetting.Register, Status);
            }
        }
    
        // Set accelerometer to standby
        RegisterSetting = { ADXL345_POWER_CTL, ADXL345_POWER_CTL_STANDBY };
        Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
        WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
        if (!NT_SUCCESS(Status)) 
        {
            TraceError("ACC %!FUNC! I2CSensorWriteRegister to 0x%02x failed! %!STATUS!", RegisterSetting.Register, Status);
        }
        else
        {
            InitPropVariantFromUInt32(SensorState_Idle, &(pAccDevice->m_pSensorProperties->List[SENSOR_PROPERTY_STATE].Value));
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get supported data fields. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS ADXL345AccDevice::OnGetSupportedDataFields(
    _In_ SENSOROBJECT SensorInstance,          // Sensor device object
    _Inout_opt_ PSENSOR_PROPERTY_LIST pFields, // Pointer to a list of supported properties
    _Out_ PULONG pSize)                        // Number of bytes for the list of supported properties
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
    }
    else
    {
        *pSize = 0;

        // Get the device context
        PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! Invalid parameters! %!STATUS!", Status);
        }
        else if (nullptr == pFields)
        {
            // Just return size
            *pSize = pAccDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
        }
        else 
        {
            if (pFields->AllocatedSizeInBytes < pAccDevice->m_pSupportedDataFields->AllocatedSizeInBytes)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("ACC %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            }
            else
            {
                // Fill out data
                Status = PropertiesListCopy(pFields, pAccDevice->m_pSupportedDataFields);
                if (!NT_SUCCESS(Status))
                {
                    TraceError("ACC %!FUNC! PropertiesListCopy failed %!STATUS!", Status);
                }
                else
                {
                    *pSize = pAccDevice->m_pSupportedDataFields->AllocatedSizeInBytes;
                }
            }
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get sensor properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS ADXL345AccDevice::OnGetProperties(
    _In_ SENSOROBJECT SensorInstance,                // Sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // Pointer to a list of sensor properties
    _Out_ PULONG pSize)                              // Number of bytes for the list of sensor properties
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
    }
    else
    {
        *pSize = 0;

        // Get the device context
        PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! Invalid parameters! %!STATUS!", Status);
        }
        else if (nullptr == pProperties)
        {
            // Just return size
            *pSize = CollectionsListGetMarshalledSize(pAccDevice->m_pSensorProperties);
        }
        else
        {
            if (pProperties->AllocatedSizeInBytes < CollectionsListGetMarshalledSize(pAccDevice->m_pSensorProperties))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("ACC %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            }
            else
            {
                // Fill out all data
                Status = CollectionsListCopyAndMarshall(pProperties, pAccDevice->m_pSensorProperties);
                if (!NT_SUCCESS(Status))
                {
                    TraceError("ACC %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
                }
                else
                {
                    *pSize = CollectionsListGetMarshalledSize(pAccDevice->m_pSensorProperties);
                }
            }
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get data field properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS ADXL345AccDevice::OnGetDataFieldProperties(
    _In_ SENSOROBJECT SensorInstance,                   // Sensor device object
    _In_ const PROPERTYKEY *pDataField,                 // Pointer to the propertykey of requested property
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties,    // Pointer to a list of sensor properties
    _Out_ PULONG pSize)                                 // Number of bytes for the list of sensor properties
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
    }
    else
    {
        *pSize = 0;

        // Get the device context
        PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice || nullptr == pDataField)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! Invalid parameters! %!STATUS!", Status);
        }
        else if (!IsEqualPropertyKey(*pDataField, pAccDevice->m_pSupportedDataFields->List[SENSOR_DATA_ACCELERATION_X_G]) &&
                 !IsEqualPropertyKey(*pDataField, pAccDevice->m_pSupportedDataFields->List[SENSOR_DATA_ACCELERATION_Y_G]) &&
                 !IsEqualPropertyKey(*pDataField, pAccDevice->m_pSupportedDataFields->List[SENSOR_DATA_ACCELERATION_Z_G]))
        {
            Status = STATUS_NOT_SUPPORTED;
            TraceError("ACC %!FUNC! ADXL345 does NOT have properties for this data field. Failed %!STATUS!", Status);
        }
        else if (nullptr == pProperties)
        {
            // Just return size
            *pSize = CollectionsListGetMarshalledSize(pAccDevice->m_pDataFieldProperties);
        }
        else
        {
            if (pProperties->AllocatedSizeInBytes < CollectionsListGetMarshalledSize(pAccDevice->m_pDataFieldProperties))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("ACC %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            }
            else
            {
                // Fill out all data
                Status = CollectionsListCopyAndMarshall(pProperties, pAccDevice->m_pDataFieldProperties);
                if (!NT_SUCCESS(Status))
                {
                    TraceError("ACC %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
                }
                else
                {
                    *pSize = CollectionsListGetMarshalledSize(pAccDevice->m_pDataFieldProperties);
                }
            }
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get sampling rate of the sensor.
NTSTATUS ADXL345AccDevice::OnGetDataInterval(
    _In_ SENSOROBJECT SensorInstance,   // Sensor device object
    _Out_ PULONG pDataRateMs)           // Sampling rate in milliseconds
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
    if (nullptr == pAccDevice)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! Invalid parameters! %!STATUS!", Status);
    }
    else if (nullptr == pDataRateMs) 
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! Invalid parameters! %!STATUS!", Status);
    }
    else
    {
        *pDataRateMs = pAccDevice->m_Interval;
        TraceInformation("%!FUNC! giving data rate %lu", *pDataRateMs);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Return the rate that is just less than the desired report interval
inline DATA_RATE _GetDataRateFromReportInterval(_In_ ULONG ReportInterval)
{
    DATA_RATE dataRate = ACCELEROMETER_SUPPORTED_DATA_RATES[0];

    for (ULONG i = 0; i < ACCELEROMETER_SUPPORTED_DATA_RATES_COUNT; i++)
    {
        dataRate = ACCELEROMETER_SUPPORTED_DATA_RATES[i];
        if (dataRate.DataRateInterval <= ReportInterval)
        {
            break;
        }
    }

    return dataRate;
}

// Called by Sensor CLX to set sampling rate of the sensor.
NTSTATUS ADXL345AccDevice::OnSetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _In_ ULONG DataRateMs)            // Sampling rate in milliseconds
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    // Get the device context
    PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
    if (nullptr == pAccDevice || DataRateMs < ACCELEROMETER_MIN_REPORT_INTERVAL)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! Invalid parameters! %!STATUS!", Status);
    }
    else
    {
        pAccDevice->m_Interval = DataRateMs;
        if (pAccDevice->m_Started) 
        {
            pAccDevice->m_Started = false;
        
            WdfWaitLockAcquire(pAccDevice->m_I2CWaitLock, NULL);
        
            // Disable Interrupts
            REGISTER_SETTING RegisterSetting = { ADXL345_INT_ENABLE, 0 };
            Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
            if (!NT_SUCCESS(Status))
            {
                WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
                TraceError("ACC %!FUNC! Failed to disable interrupts. %!STATUS!", Status);
            }

            // Update data rate in HW
            else // (if NT_SUCCESS(Status))
            {
                RegisterSetting = { ADXL345_BW_RATE, _GetDataRateFromReportInterval(DataRateMs).RateCode };
                Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
                if (!NT_SUCCESS(Status))
                {
                    WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
                    TraceError("ACC %!FUNC! I2CSensorWriteRegister to 0x%02x failed! %!STATUS!", RegisterSetting.Register, Status);
                }
            }

            // Re-enable Interrupts
            if (NT_SUCCESS(Status))
            {
                pAccDevice->m_Started = true;
                pAccDevice->m_FirstSample = true;
                
                RegisterSetting = { ADXL345_INT_ENABLE, ADXL345_INT_ACTIVITY };
                Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
                WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
                if (!NT_SUCCESS(Status))
                {
                    TraceError("ACC %!FUNC! Failed to re-enable interrupts. %!STATUS!", Status);
                }
            }
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get data thresholds. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size
// for the buffer, allocate buffer, then call the function again to retrieve
// sensor information.
NTSTATUS ADXL345AccDevice::OnGetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,                   // Sensor Device Object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pThresholds,    // Pointer to a list of sensor thresholds
    _Out_ PULONG pSize)                                 // Number of bytes for the list of sensor thresholds
{

    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    if (nullptr == pSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! pSize: Invalid parameter! %!STATUS!", Status);
    }
    else
    {
        *pSize = 0;
    
        PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! Invalid parameters! %!STATUS!", Status);
        }
        else if (nullptr == pThresholds)
        {
            // Just return size
            *pSize = CollectionsListGetMarshalledSize(pAccDevice->m_pThresholds);
        }
        else
        {
            if (pThresholds->AllocatedSizeInBytes < CollectionsListGetMarshalledSize(pAccDevice->m_pThresholds))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceError("ACC %!FUNC! Buffer is too small. Failed %!STATUS!", Status);
            }
            else
            {
                // Fill out all data
                Status = CollectionsListCopyAndMarshall(pThresholds, pAccDevice->m_pThresholds);
                if (!NT_SUCCESS(Status))
                {
                    TraceError("ACC %!FUNC! CollectionsListCopyAndMarshall failed %!STATUS!", Status);
                }
                else
                {
                    *pSize = CollectionsListGetMarshalledSize(pAccDevice->m_pThresholds);
                }
            }
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to set data thresholds.
NTSTATUS ADXL345AccDevice::OnSetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,           // Sensor Device Object
    _In_ PSENSOR_COLLECTION_LIST pThresholds)   // Pointer to a list of sensor thresholds
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    PADXL345AccDevice pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
    if (nullptr == pAccDevice) 
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! Sensor(%08X) parameter is invalid %!STATUS!", (INT) SensorInstance, Status);
    }

    else // if (NT_SUCCESS(Status))
    {
        for (ULONG i = 0; i < pThresholds->Count; i++)
        {
            Status = PropKeyFindKeySetPropVariant(pAccDevice->m_pThresholds, &(pThresholds->List[i].Key), true, &(pThresholds->List[i].Value));
            if (!NT_SUCCESS(Status))
            {
                Status = STATUS_INVALID_PARAMETER;
                TraceError("ACC %!FUNC! Adxl345 does NOT have threshold for this data field. Failed %!STATUS!", Status);
                break;
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = PropKeyFindKeyGetFloat(pAccDevice->m_pThresholds, &PKEY_SensorData_AccelerationX_Gs, &(pAccDevice->m_CachedThresholds.X));
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! PropKeyFindKeyGetFloat for X failed! %!STATUS!", Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = PropKeyFindKeyGetFloat(pAccDevice->m_pThresholds, &PKEY_SensorData_AccelerationY_Gs, &(pAccDevice->m_CachedThresholds.Y));
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! PropKeyFindKeyGetFloat for Y failed! %!STATUS!", Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = PropKeyFindKeyGetFloat(pAccDevice->m_pThresholds, &PKEY_SensorData_AccelerationZ_Gs, &(pAccDevice->m_CachedThresholds.Z));
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! PropKeyFindKeyGetFloat for Z failed! %!STATUS!", Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        // The accelerometer only supports a single value, so pick the smallest, i.e. most sensitive.
        FLOAT MinThreshold = min(pAccDevice->m_CachedThresholds.X, min(pAccDevice->m_CachedThresholds.Y, pAccDevice->m_CachedThresholds.Z));
        MinThreshold = max(MinThreshold, static_cast<FLOAT>(ACCELEROMETER_MIN_CHANGE_SENSITIVITY));
    
        // Disable Interrupts
        REGISTER_SETTING RegisterSetting = { ADXL345_INT_ENABLE, 0 };
        WdfWaitLockAcquire(pAccDevice->m_I2CWaitLock, NULL);
        Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
        if (!NT_SUCCESS(Status))
        {
            WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
            TraceError("ACC %!FUNC! Failed to disable interrupts. %!STATUS!", Status);
        }

        // The threshold can only be set in increments, so round down to a more sensitive setting for hardware
        else //if (NT_SUCCESS(Status))
        {
            BYTE NewThreshold = static_cast<BYTE>(MinThreshold / ACCELEROMETER_CHANGE_SENSITIVITY_RESOLUTION);
            RegisterSetting = { ADXL345_THRESH_ACT, NewThreshold };
            Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
            if (!NT_SUCCESS(Status))
            {
                WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
                TraceError("ACC %!FUNC! I2CSensorWriteRegister to 0x%02x failed! %!STATUS!", RegisterSetting.Register, Status);
            }
        }

        // Re-enable Interrupts
        if (NT_SUCCESS(Status))
        {
            RegisterSetting = { ADXL345_INT_ENABLE, ADXL345_INT_ACTIVITY };
            Status = I2CSensorWriteRegister(pAccDevice->m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
            WdfWaitLockRelease(pAccDevice->m_I2CWaitLock);
            if (!NT_SUCCESS(Status))
            {
                TraceError("ACC %!FUNC! Failed to re-enable interrupts. %!STATUS!", Status);
            }
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to handle IOCTLs that clx does not support
NTSTATUS ADXL345AccDevice::OnIoControl(
    _In_ SENSOROBJECT /*SensorInstance*/, // WDF queue object
    _In_ WDFREQUEST /*Request*/,          // WDF request object
    _In_ size_t /*OutputBufferLength*/,   // number of bytes to retrieve from output buffer
    _In_ size_t /*InputBufferLength*/,    // number of bytes to retrieve from input buffer
    _In_ ULONG /*IoControlCode*/)         // IOCTL control code
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}
