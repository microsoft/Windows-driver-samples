/*++

Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Device.c

Abstract:

    This module contains the implementation of the VirtualSerial sample
    driver's device callback object.

    The VirtualSerial sample device does very little.  It does not implement
    either of the PNP interfaces so once the device is setup, it won't ever get
    any callbacks until the device is removed.

Environment:

    Windows Driver Framework

--*/

#include "internal.h"

NTSTATUS
DeviceCreate(
    _In_  WDFDRIVER         Driver,
    _In_  PWDFDEVICE_INIT   DeviceInit,
    _Out_ PDEVICE_CONTEXT   *DeviceContext
    )
/*++

  Routine Description:

    This method creates and initializs an instance of the VirtualSerial driver's
    device callback object.

  Arguments:

    FxDeviceInit - the settings for the device.

    Device - a location to store the referenced pointer to the device object.

  Return Value:

    Status

--*/
{
    NTSTATUS                status;
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    WDFDEVICE               device;
    PDEVICE_CONTEXT         deviceContext;
    UNREFERENCED_PARAMETER  (Driver);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
                            &deviceAttributes,
                            DEVICE_CONTEXT);

    deviceAttributes.SynchronizationScope = WdfSynchronizationScopeDevice;
    deviceAttributes.EvtCleanupCallback   = EvtDeviceCleanup;

    status = WdfDeviceCreate(&DeviceInit,
                            &deviceAttributes,
                            &device);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfDeviceCreate failed 0x%x", status);
        return status;
    }

    deviceContext = GetDeviceContext(device);
    deviceContext->Device = device;

    *DeviceContext = deviceContext;

    return status;
}


NTSTATUS
DeviceConfigure(
    _In_  PDEVICE_CONTEXT   DeviceContext
    )
/*++

  Routine Description:

    This method is called after the device callback object has been initialized
    and returned to the driver.  It would setup the device's queues and their
    corresponding callback objects.

  Arguments:

    FxDevice - the framework device object for which we're handling events.

  Return Value:

    status

--*/
{
    NTSTATUS                status;
    WDFDEVICE               device = DeviceContext->Device;
    WDFKEY                  key;
    LPGUID                  guid;
    errno_t                 errorNo;
    
    DECLARE_CONST_UNICODE_STRING(portName,          REG_VALUENAME_PORTNAME);
    DECLARE_UNICODE_STRING_SIZE (comPort,           10);
    DECLARE_UNICODE_STRING_SIZE (symbolicLinkName,  SYMBOLIC_LINK_NAME_LENGTH);

#ifdef _FAKE_MODEM
    //
    // Compiled as fake modem
    //
    guid = (LPGUID) &GUID_DEVINTERFACE_MODEM;
#else
    //
    // Compiled as virtual serial port
    //
    guid = (LPGUID) &GUID_DEVINTERFACE_COMPORT;
#endif

    //
    // Create device interface
    //
    status = WdfDeviceCreateDeviceInterface(
                            device,
                            guid,
                            NULL);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: Cannot create device interface");
        goto Exit;
    }

    //
    // Read the COM port number from the registry, which has been automatically
    // created by "MsPorts!PortsClassInstaller" if INF file says "Class=Ports"
    //
    status = WdfDeviceOpenRegistryKey(
                            device,
                            PLUGPLAY_REGKEY_DEVICE,
                            KEY_QUERY_VALUE,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &key);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: Failed to retrieve device hardware key root");
        goto Exit;
    }

    status = WdfRegistryQueryUnicodeString(
                            key,
                            &portName,
                            NULL,
                            &comPort);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: Failed to read PortName");
        goto Exit;
    }
        
    //
    // Manually create the symbolic link name. Length is the length in
    // bytes not including the NULL terminator.
    //
    // 6054 and 26035 are code analysis warnings that comPort.Buffer might
    // not be NULL terminated, while we know that they are. 
    //
    #pragma warning(suppress: 6054 26035)
    symbolicLinkName.Length = (USHORT)((wcslen(comPort.Buffer) * sizeof(wchar_t))
                                + sizeof(SYMBOLIC_LINK_NAME_PREFIX) - sizeof(UNICODE_NULL));
                                
    if (symbolicLinkName.Length >= symbolicLinkName.MaximumLength) {
        
        Trace(TRACE_LEVEL_ERROR, "Error: Buffer overflow when creating COM port name. Size"
            " is %d, buffer length is %d", symbolicLinkName.Length, symbolicLinkName.MaximumLength);
        status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }
        
    errorNo = wcscpy_s(symbolicLinkName.Buffer,
                       SYMBOLIC_LINK_NAME_LENGTH,
                       SYMBOLIC_LINK_NAME_PREFIX);
                           
    if (errorNo != 0) {
        Trace(TRACE_LEVEL_ERROR, 
              "Failed to copy %ws to buffer with error %d",
              SYMBOLIC_LINK_NAME_PREFIX, errorNo);
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
        
    errorNo = wcscat_s(symbolicLinkName.Buffer,
                       SYMBOLIC_LINK_NAME_LENGTH,
                       comPort.Buffer);
                           
    if (errorNo != 0) {
        Trace(TRACE_LEVEL_ERROR, 
              "Failed to copy %ws to buffer with error %d",
              comPort.Buffer, errorNo);
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
                                
    //
    // Create symbolic link
    //
    status = WdfDeviceCreateSymbolicLink(
                            device,
                            &symbolicLinkName);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: Cannot create symbolic link %ws", symbolicLinkName.Buffer);
        goto Exit;
    }

    status = DeviceGetPdoName(DeviceContext);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    status = DeviceWriteLegacyHardwareKey(
                            DeviceContext->PdoName,
                            comPort.Buffer,
                            DeviceContext->Device);
    if (NT_SUCCESS(status)) {
        DeviceContext->CreatedLegacyHardwareKey = TRUE;
    }

    status = QueueCreate(DeviceContext);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

Exit:
    return status;
}


NTSTATUS
DeviceGetPdoName(
    _In_  PDEVICE_CONTEXT   DeviceContext
    )
{
    NTSTATUS                status;
    WDFDEVICE               device = DeviceContext->Device;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFMEMORY               memory;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfDeviceAllocAndQueryProperty(
                            device,
                            DevicePropertyPhysicalDeviceObjectName,
                            NonPagedPool,
                            &attributes,
                            &memory);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: Failed to query PDO name");
        goto Exit;
    }

    DeviceContext->PdoName = (PWCHAR) WdfMemoryGetBuffer(memory, NULL);
    Trace(TRACE_LEVEL_ERROR,
            "PDO Name is %ws", DeviceContext->PdoName);

Exit:
    return status;
}


NTSTATUS
DeviceWriteLegacyHardwareKey(
    _In_  PWSTR             PdoName,
    _In_  PWSTR             ComPort,
    _In_  WDFDEVICE         Device
    )
{
    WDFKEY                  key = NULL;
    NTSTATUS                status;
    UNICODE_STRING          pdoString = {0};
    UNICODE_STRING          comPort = {0};
    
    DECLARE_CONST_UNICODE_STRING(deviceSubkey, SERIAL_DEVICE_MAP);
                
    RtlInitUnicodeString(&pdoString, PdoName);  
    RtlInitUnicodeString(&comPort, ComPort);
   
    status = WdfDeviceOpenDevicemapKey(Device,
                                       &deviceSubkey,
                                       KEY_SET_VALUE,
                                       WDF_NO_OBJECT_ATTRIBUTES,
                                       &key);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: Failed to open DEVICEMAP\\SERIALCOMM key 0x%x", status);
        goto exit;
    }
    
    status = WdfRegistryAssignUnicodeString(key,
                                            &pdoString,
                                            &comPort);
                                            
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: Failed to write to DEVICEMAP\\SERIALCOMM key 0x%x", status);
        goto exit;
    }                                 

exit:

    if (key != NULL) {
        WdfRegistryClose(key);
        key = NULL;
    }

    return status;
}


VOID
EvtDeviceCleanup(
    _In_  WDFOBJECT         Object
    )
{
    WDFDEVICE               device = (WDFDEVICE) Object;
    PDEVICE_CONTEXT         deviceContext = GetDeviceContext(device);
    NTSTATUS                status;
    WDFKEY                  key = NULL;
    UNICODE_STRING          pdoString = {0};
    
    DECLARE_CONST_UNICODE_STRING(deviceSubkey, SERIAL_DEVICE_MAP);

    if (deviceContext->CreatedLegacyHardwareKey == TRUE) {
    
        RtlInitUnicodeString(&pdoString, deviceContext->PdoName);
    
        status = WdfDeviceOpenDevicemapKey(device,
                                           &deviceSubkey,
                                           KEY_SET_VALUE,
                                           WDF_NO_OBJECT_ATTRIBUTES,
                                           &key);
                                           
        if (!NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR,
                "Error: Failed to open DEVICEMAP\\SERIALCOMM key 0x%x", status);
            goto exit;
        }
        
        status = WdfRegistryRemoveValue(key,
                                        &pdoString);
        if (!NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR,
                "Error: Failed to delete %S key, 0x%x", pdoString.Buffer, status);  
            goto exit;
        }
        
        status = WdfRegistryRemoveKey(key);
        if (!NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR,
                "Error: Failed to delete %S, 0x%x", SERIAL_DEVICE_MAP, status);  
            goto exit;
        }           
    }
        
exit:

    if (key != NULL) {
        WdfRegistryClose(key);
        key = NULL;
    }

    return;
}


ULONG
GetBaudRate(
    _In_  PDEVICE_CONTEXT   DeviceContext
    )
{
    return DeviceContext->BaudRate;
}

VOID
SetBaudRate(
    _In_  PDEVICE_CONTEXT   DeviceContext,
    _In_  ULONG             BaudRate
    )
{
    DeviceContext->BaudRate = BaudRate;
}

ULONG *
GetModemControlRegisterPtr(
    _In_  PDEVICE_CONTEXT   DeviceContext
    )
{
    return &DeviceContext->ModemControlRegister;
}

ULONG *
GetFifoControlRegisterPtr(
    _In_  PDEVICE_CONTEXT   DeviceContext
    )
{
    return &DeviceContext->FifoControlRegister;
}

ULONG *
GetLineControlRegisterPtr(
    _In_  PDEVICE_CONTEXT   DeviceContext
    )
{
    return &DeviceContext->LineControlRegister;
}

VOID
SetValidDataMask(
    _In_  PDEVICE_CONTEXT   DeviceContext,
    _In_  UCHAR             Mask
    )
{
    DeviceContext->ValidDataMask = Mask;
}

VOID
SetTimeouts(
    _In_  PDEVICE_CONTEXT   DeviceContext,
    _In_  SERIAL_TIMEOUTS   Timeouts
    )
{
    DeviceContext->Timeouts = Timeouts;
}

VOID
GetTimeouts(
    _In_  PDEVICE_CONTEXT   DeviceContext,
    _Out_ SERIAL_TIMEOUTS   *Timeouts
    )
{
    *Timeouts = DeviceContext->Timeouts;
}
