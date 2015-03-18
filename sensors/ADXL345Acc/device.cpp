//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of WDF callback functions 
//    for the ADXL345 accelerometer sensor driver.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"

#include "Device.tmh"


// This routine is the AddDevice entry point for the custom sensor client
// driver. This routine is called by the framework in response to AddDevice
// call from the PnP manager. It will create and initialize the device object
// to represent a new instance of the sensor client.
NTSTATUS ADXL345AccDevice::OnDeviceAdd(
    _In_    WDFDRIVER /* Driver */,         // Supplies a handle to the driver object created in DriverEntry
    _Inout_ PWDFDEVICE_INIT pAccDeviceInit) // Supplies a pointer to a framework-allocated WDFDEVICE_INIT structure
{
    WDFDEVICE Device = nullptr;

    SENSOR_FunctionEnter();

    WdfDeviceInitSetPowerPolicyOwnership(pAccDeviceInit, true);

    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&FdoAttributes);

    // Initialize FDO (functional device object) attributes and set up file object with sensor extension
    NTSTATUS Status = SensorsCxDeviceInitConfig(pAccDeviceInit, &FdoAttributes, 0);
    if (!NT_SUCCESS(Status))
    {
        TraceError("ACC %!FUNC! SensorsCxDeviceInitConfig failed %!STATUS!", Status);
    }

    else // if (NT_SUCCESS(Status))
    {
        // Register the PnP callbacks with the framework.
        WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
        pnpPowerCallbacks.EvtDevicePrepareHardware = ADXL345AccDevice::OnPrepareHardware;
        pnpPowerCallbacks.EvtDeviceReleaseHardware = ADXL345AccDevice::OnReleaseHardware;
        pnpPowerCallbacks.EvtDeviceD0Entry = ADXL345AccDevice::OnD0Entry;
        pnpPowerCallbacks.EvtDeviceD0Exit = ADXL345AccDevice::OnD0Exit;
    
        WdfDeviceInitSetPnpPowerEventCallbacks(pAccDeviceInit, &pnpPowerCallbacks);
    
        // Call the framework to create the device
        Status = WdfDeviceCreate(&pAccDeviceInit, &FdoAttributes, &Device);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! WdfDeviceCreate failed %!STATUS!", Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        // Register CLX callback function pointers
        SENSOR_CONTROLLER_CONFIG SensorConfig;
        SENSOR_CONTROLLER_CONFIG_INIT(&SensorConfig);
        SensorConfig.DriverIsPowerPolicyOwner = WdfUseDefault;
    
        SensorConfig.EvtSensorStart = ADXL345AccDevice::OnStart;
        SensorConfig.EvtSensorStop = ADXL345AccDevice::OnStop;
        SensorConfig.EvtSensorGetSupportedDataFields = ADXL345AccDevice::OnGetSupportedDataFields;
        SensorConfig.EvtSensorGetDataInterval = ADXL345AccDevice::OnGetDataInterval;
        SensorConfig.EvtSensorSetDataInterval = ADXL345AccDevice::OnSetDataInterval;
        SensorConfig.EvtSensorGetDataFieldProperties = ADXL345AccDevice::OnGetDataFieldProperties;
        SensorConfig.EvtSensorGetDataThresholds = ADXL345AccDevice::OnGetDataThresholds;
        SensorConfig.EvtSensorSetDataThresholds = ADXL345AccDevice::OnSetDataThresholds;
        SensorConfig.EvtSensorGetProperties = ADXL345AccDevice::OnGetProperties;
        SensorConfig.EvtSensorDeviceIoControl = ADXL345AccDevice::OnIoControl;
    
        // Initialize the sensor device with the Sensor CLX
        // This lets the CLX call the above callbacks when
        // necessary and allows applications to retrieve and
        // set device data.
        Status = SensorsCxDeviceInitialize(Device, &SensorConfig);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! SensorsCxDeviceInitialize failed %!STATUS!", Status);
        }
    }

    // Ensure device is disable-able
    // By default, devices enumerated by ACPI are not disable-able
    // Our accelerometer is enumerated by the ACPI so we must
    // explicitly make it disable-able.
    if (NT_SUCCESS(Status))
    {
        WDF_DEVICE_STATE DeviceState;
        WDF_DEVICE_STATE_INIT(&DeviceState);
        DeviceState.NotDisableable = WdfFalse;
        WdfDeviceSetDeviceState(Device, &DeviceState);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is called by the framework when the PnP manager sends an
// IRP_MN_START_DEVICE request to the driver stack. This routine is
// responsible for performing operations that are necessary to make the
// driver's device operational (for e.g. mapping the hardware resources
// into memory).
NTSTATUS ADXL345AccDevice::OnPrepareHardware(
    _In_ WDFDEVICE Device,                  // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST ResourcesRaw,         // Supplies a handle to a collection of framework resource
                                            // objects. This collection identifies the raw (bus-relative) hardware
                                            // resources that have been assigned to the device.
    _In_ WDFCMRESLIST ResourcesTranslated)  // Supplies a handle to a collection of framework
                                            // resource objects. This collection identifies the translated
                                            // (system-physical) hardware resources that have been assigned to the
                                            // device. The resources appear from the CPU's point of view.
{
    PADXL345AccDevice pAccDevice = nullptr;
    
    SENSOR_FunctionEnter();

    // Create WDFOBJECT for the sensor
    WDF_OBJECT_ATTRIBUTES sensorAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&sensorAttributes, ADXL345AccDevice);

    // Register sensor instance with clx
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxSensorCreate(Device, &sensorAttributes, &SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("ACC %!FUNC! SensorsCxSensorCreate failed %!STATUS!", Status);
    }

    else // if (NT_SUCCESS(Status))
    {    
        pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("ACC %!FUNC! SensorsCxSensorCreate failed %!STATUS!", Status);
        }
    }

    // Fill out sensor context
    if (NT_SUCCESS(Status))
    {
        Status = pAccDevice->Initialize(Device, SensorInstance);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! Initialize device object failed %!STATUS!", Status);
        }
    }
        
    // Initialize sensor instance with clx
    if (NT_SUCCESS(Status))
    {
        SENSOR_CONFIG SensorConfig;
        SENSOR_CONFIG_INIT(&SensorConfig);
        SensorConfig.pEnumerationList = pAccDevice->m_pEnumerationProperties;
        Status = SensorsCxSensorInitialize(SensorInstance, &SensorConfig);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! SensorsCxSensorInitialize failed %!STATUS!", Status);
        }
    }
    
    // ACPI and IoTarget configuration
    if (NT_SUCCESS(Status))
    {
        Status = pAccDevice->ConfigureIoTarget(ResourcesRaw, ResourcesTranslated);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! Failed to configure IoTarget %!STATUS!", Status);
        }
    }

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
NTSTATUS ADXL345AccDevice::OnReleaseHardware(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
{
    PADXL345AccDevice pAccDevice = nullptr;

    SENSOR_FunctionEnter();

    // Get the sensor instance
    ULONG SensorInstanceCount = 1;
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) || 0 == SensorInstanceCount || NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
    }

    else // if (NT_SUCCESS(Status))
    {
        pAccDevice = GetADXL345AccContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! GetADXL345AccContextFromSensorInstance failed %!STATUS!", Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        pAccDevice->DeInit();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is invoked by the framework to program the device to goto 
// D0, which is the working state. The framework invokes callback every
// time the hardware needs to be (re-)initialized.  This includes after
// IRP_MN_START_DEVICE, IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE,
// and IRP_MN_SET_POWER-D0.
NTSTATUS ADXL345AccDevice::OnD0Entry(
    _In_  WDFDEVICE Device,                         // Supplies a handle to the framework device object
    _In_  WDF_POWER_DEVICE_STATE /*PreviousState*/) // WDF_POWER_DEVICE_STATE-typed enumerator that identifies
                                                    // the device power state that the device was in before this transition to D0
{
    PADXL345AccDevice pAccDevice = nullptr;

    SENSOR_FunctionEnter();

    // Get the sensor instance
    ULONG SensorInstanceCount = 1;
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
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

    if (NT_SUCCESS(Status))
    {
        Status = pAccDevice->PowerOn();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is invoked by the framework to program the device to go into
// a certain Dx state. The framework invokes callback every the the device is 
// leaving the D0 state, which happens when the device is stopped, when it is 
// removed, and when it is powered off. EvtDeviceD0Exit event callback must 
// perform any operations that are necessary before the specified device is 
// moved out of the D0 state.  If the driver needs to save hardware state 
// before the device is powered down, then that should be done here.
NTSTATUS ADXL345AccDevice::OnD0Exit(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE)/*TargetState*/ // Supplies the device power state which the device will be put
                                                // in once the callback is complete
{
    PADXL345AccDevice pAccDevice = nullptr;

    SENSOR_FunctionEnter();

    // Get the sensor instance
    ULONG SensorInstanceCount = 1;
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
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

    if (NT_SUCCESS(Status))
    {
        Status = pAccDevice->PowerOff();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Get the HW resource from the ACPI, then configure and store the IoTarget
NTSTATUS ADXL345AccDevice::ConfigureIoTarget(
    _In_ WDFCMRESLIST ResourcesRaw,         // Supplies a handle to a collection of framework resource
                                            // objects. This collection identifies the raw (bus-relative) hardware
                                            // resources that have been assigned to the device.
    _In_ WDFCMRESLIST ResourcesTranslated)  // Supplies a handle to a collection of framework
                                            // resource objects. This collection identifies the translated
                                            // (system-physical) hardware resources that have been assigned to the
                                            // device. The resources appear from the CPU's point of view.
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG I2CConnectionResourceCount = 0;
    LARGE_INTEGER I2CConnId = {};

    DECLARE_UNICODE_STRING_SIZE(deviceName, RESOURCE_HUB_PATH_SIZE);

    SENSOR_FunctionEnter();

    // Get hardware resource from ACPI and set up IO target
    ULONG ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    for (ULONG i = 0; i < ResourceCount; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR DescriptorRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, i);
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);
        switch (Descriptor->Type) 
        {
            // Check we have I2C bus assigned in ACPI
            case CmResourceTypeConnection:
                TraceInformation("ACC %!FUNC! I2C resource found.");
                if (Descriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
                    Descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C) 
                {
                    I2CConnId.LowPart = Descriptor->u.Connection.IdLowPart;
                    I2CConnId.HighPart = Descriptor->u.Connection.IdHighPart;
                    I2CConnectionResourceCount++;
                }
                break;
    
            // Check we have an interrupt assigned in ACPI and create interrupt
            case CmResourceTypeInterrupt:
                TraceInformation("ACC %!FUNC! GPIO interrupt resource found.");
                WDF_INTERRUPT_CONFIG InterruptConfig;
                WDF_INTERRUPT_CONFIG_INIT(&InterruptConfig, OnInterruptIsr, NULL);
                InterruptConfig.InterruptRaw = DescriptorRaw;
                InterruptConfig.InterruptTranslated = Descriptor;
                // Configure an interrupt work item which runs at IRQL = PASSIVE_LEVEL
                // Note: to configure to run at IRQL = DISPATCH_LEVEL, set up an InterruptDpc instead of an InterruptWorkItem
                InterruptConfig.EvtInterruptWorkItem = OnInterruptWorkItem;
                InterruptConfig.PassiveHandling = true;
                Status = WdfInterruptCreate(m_Device, &InterruptConfig, WDF_NO_OBJECT_ATTRIBUTES, &m_Interrupt);
                if (!NT_SUCCESS(Status))
                {
                    TraceError("ACC %!FUNC! WdfInterruptCreate failed %!STATUS!", Status);
                }
                break;

            default:
                break;
        }
    }

    if (NT_SUCCESS(Status) && I2CConnectionResourceCount != 1)
    {
        Status = STATUS_UNSUCCESSFUL;
        TraceError("ACC %!FUNC! Did not find I2C resource! %!STATUS!", Status);
    }

    // Set up I2C I/O target. Issued with I2C R/W transfers
    if (NT_SUCCESS(Status))
    {
        m_I2CIoTarget = NULL;
        Status = WdfIoTargetCreate(m_Device, WDF_NO_OBJECT_ATTRIBUTES, &m_I2CIoTarget);
    
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! WdfIoTargetCreate failed! %!STATUS!", Status);
        }
    }

    // Setup Target string (\\\\.\\RESOURCE_HUB\\<ConnID from ResHub>
    if (NT_SUCCESS(Status))
    {
        Status = StringCbPrintfW(deviceName.Buffer, RESOURCE_HUB_PATH_SIZE, L"%s\\%0*I64x", RESOURCE_HUB_DEVICE_NAME, static_cast<unsigned int>(sizeof(LARGE_INTEGER) * 2), I2CConnId.QuadPart);
        deviceName.Length = _countof(deviceName_buffer);
    
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! RESOURCE_HUB_CREATE_PATH_FROM_ID failed!");
        }
    }

    // Connect to I2C target
    if (NT_SUCCESS(Status))
    {
        WDF_IO_TARGET_OPEN_PARAMS OpenParams;
        WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams, &deviceName, FILE_ALL_ACCESS);
    
        Status = WdfIoTargetOpen(m_I2CIoTarget, &OpenParams);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! WdfIoTargetOpen failed! %!STATUS!", Status);
        }
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Write the default device configuration to the device
NTSTATUS ADXL345AccDevice::PowerOn()
{
    NTSTATUS Status = STATUS_SUCCESS;

    WdfWaitLockAcquire(m_I2CWaitLock, NULL);
    
    for (DWORD i = 0; i < ARRAYSIZE(g_ConfigurationSettings); i++)
    {
        REGISTER_SETTING setting = g_ConfigurationSettings[i];
        Status = I2CSensorWriteRegister(m_I2CIoTarget, setting.Register, &setting.Value, sizeof(setting.Value));
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! I2CSensorReadRegister from 0x%02x failed! %!STATUS!", setting.Register, Status);
            WdfWaitLockRelease(m_I2CWaitLock);
            return Status;
        }
    }

    WdfWaitLockRelease(m_I2CWaitLock);

    InitPropVariantFromUInt32(SensorState_Idle, &(m_pSensorProperties->List[SENSOR_PROPERTY_STATE].Value));
    m_PoweredOn = true;

    return Status;
}

NTSTATUS ADXL345AccDevice::PowerOff()
{
    REGISTER_SETTING RegisterSetting = { ADXL345_POWER_CTL, ADXL345_POWER_CTL_STANDBY };
    
    WdfWaitLockAcquire(m_I2CWaitLock, NULL);
    NTSTATUS Status = I2CSensorWriteRegister(m_I2CIoTarget, RegisterSetting.Register, &RegisterSetting.Value, sizeof(RegisterSetting.Value));
    WdfWaitLockRelease(m_I2CWaitLock);
        
    if (!NT_SUCCESS(Status))
    {
        TraceError("ACC %!FUNC! Failed to put device into standby %!STATUS!", Status);
    }
    else
    {
        m_PoweredOn = false;
    }

    return Status;
}
