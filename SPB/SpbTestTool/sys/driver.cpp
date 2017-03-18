/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    driver.cpp

Abstract:

    This module contains the WDF driver initialization 
    functions for the peripheral driver.

Environment:

    kernel-mode only

Revision History:

--*/

#include "internal.h"
#include "driver.h"
#include "device.h"
#include "ntstrsafe.h"

#include "driver.tmh"

NTSTATUS
#pragma prefast(suppress:__WARNING_DRIVER_FUNCTION_TYPE, "thanks, i know this already")
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG driverConfig;
    WDF_OBJECT_ATTRIBUTES driverAttributes;

    WDFDRIVER fxDriver;

    NTSTATUS status;

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    FuncEntry(TRACE_FLAG_WDFLOADING);

    WDF_DRIVER_CONFIG_INIT(&driverConfig, OnDeviceAdd);
    driverConfig.DriverPoolTag = SPBT_POOL_TAG;

    WDF_OBJECT_ATTRIBUTES_INIT(&driverAttributes);
    driverAttributes.EvtCleanupCallback = OnDriverCleanup;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &driverAttributes,
        &driverConfig,
        &fxDriver);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_WDFLOADING,
            "Error creating WDF driver object - %!STATUS!", 
            status);

        goto exit;
    }

    Trace(
        TRACE_LEVEL_VERBOSE, 
        TRACE_FLAG_WDFLOADING,
        "Created WDF driver object");

exit:

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

VOID
OnDriverCleanup(
    _In_ WDFOBJECT Object
    )
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    UNREFERENCED_PARAMETER(Object);

    WPP_CLEANUP(nullptr);

    FuncExit(TRACE_FLAG_WDFLOADING);
}

NTSTATUS
OnDeviceAdd(
    _In_    WDFDRIVER       FxDriver,
    _Inout_ PWDFDEVICE_INIT FxDeviceInit
    )
/*++
 
  Routine Description:

    This routine creates the device object for an SPB 
    controller and the device's child objects.

  Arguments:

    FxDriver - the WDF driver object handle
    FxDeviceInit - information about the PDO that we are loading on

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    PDEVICE_CONTEXT pDevice;
    NTSTATUS status;
    
    UNREFERENCED_PARAMETER(FxDriver);

    //
    // Setup PNP/Power callbacks.
    //

    {
        WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);

        pnpCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
        pnpCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;
        pnpCallbacks.EvtDeviceD0Entry = OnD0Entry;
        pnpCallbacks.EvtDeviceD0Exit = OnD0Exit;

        WdfDeviceInitSetPnpPowerEventCallbacks(FxDeviceInit, &pnpCallbacks);
    }

    //
    // Prepare for file object handling.
    //

    {
        WDF_FILEOBJECT_CONFIG fileObjectConfig;

        WDF_FILEOBJECT_CONFIG_INIT(
            &fileObjectConfig, 
            nullptr, 
            nullptr,
            OnFileCleanup);

        WDF_OBJECT_ATTRIBUTES fileObjectAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&fileObjectAttributes);

        WdfDeviceInitSetFileObjectConfig(
            FxDeviceInit, 
            &fileObjectConfig,
            &fileObjectAttributes);
    }

    //
    // Set request attributes.
    //

    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            REQUEST_CONTEXT);

        WdfDeviceInitSetRequestAttributes(FxDeviceInit, &attributes);
    }

    //
    // Create the device.
    //

    {
        WDFDEVICE fxDevice;
        WDF_OBJECT_ATTRIBUTES deviceAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

        status = WdfDeviceCreate(
            &FxDeviceInit, 
            &deviceAttributes,
            &fxDevice);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Error creating WDFDEVICE - %!STATUS!", 
                status);

            goto exit;
        }

        pDevice = GetDeviceContext(fxDevice);
        NT_ASSERT(pDevice != nullptr);

        pDevice->FxDevice = fxDevice;
    }

    //
    // Ensure device is disable-able
    //
    
    {
        WDF_DEVICE_STATE deviceState;
        WDF_DEVICE_STATE_INIT(&deviceState);
        
        deviceState.NotDisableable = WdfFalse;
        WdfDeviceSetDeviceState(pDevice->FxDevice, &deviceState);
    }

    //
    // Create queues to handle IO
    //

    {
        WDF_IO_QUEUE_CONFIG queueConfig;
        WDFQUEUE queue;

        //
        // Top-level queue
        //

        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
            &queueConfig, 
            WdfIoQueueDispatchParallel);

        queueConfig.EvtIoDefault = OnTopLevelIoDefault;
        queueConfig.PowerManaged = WdfFalse;

        status = WdfIoQueueCreate(
            pDevice->FxDevice,
            &queueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &queue
            );

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Error creating top-level IO queue - %!STATUS!", 
                status);

            goto exit;
        }

        //
        // Sequential SPB queue
        //

        WDF_IO_QUEUE_CONFIG_INIT(
            &queueConfig, 
            WdfIoQueueDispatchSequential);

        queueConfig.EvtIoRead = OnIoRead;
        queueConfig.EvtIoWrite = OnIoWrite;
        queueConfig.EvtIoDeviceControl = OnIoDeviceControl;
        queueConfig.PowerManaged = WdfFalse;

        status = WdfIoQueueCreate(
            pDevice->FxDevice,
            &queueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &pDevice->SpbQueue
            );

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Error creating SPB IO queue - %!STATUS!", 
                status);

            goto exit;
        }
    }

    //
    // Create a symbolic link.
    //

    {
        DECLARE_UNICODE_STRING_SIZE(symbolicLinkName, 128);

        status = RtlUnicodeStringPrintf(
            &symbolicLinkName, L"%ws",
            SPBTESTTOOL_SYMBOLIC_NAME);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Error creating symbolic link string for device "
                "- %!STATUS!", 
                status);

            goto exit;
        }

        status = WdfDeviceCreateSymbolicLink(
            pDevice->FxDevice, 
            &symbolicLinkName);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Error creating symbolic link for device "
                "- %!STATUS!", 
                status);

            goto exit;
        }
    }

    //
    // Retrieve registry settings.
    //

    {
        WDFKEY key = NULL;
        WDFKEY subkey = NULL;
        ULONG connectInterrupt = 0;
        NTSTATUS settingStatus;
    
        DECLARE_CONST_UNICODE_STRING(subkeyName, L"Settings");
        DECLARE_CONST_UNICODE_STRING(connectInterruptName, L"ConnectInterrupt");

        settingStatus = WdfDeviceOpenRegistryKey(
            pDevice->FxDevice,
            PLUGPLAY_REGKEY_DEVICE,
            KEY_READ,
            WDF_NO_OBJECT_ATTRIBUTES,
            &key);

        if (!NT_SUCCESS(settingStatus))
        {
            Trace(
                TRACE_LEVEL_WARNING, 
                TRACE_FLAG_WDFLOADING,
                "Error opening device registry key - %!STATUS!",
                settingStatus);
        }

        if (NT_SUCCESS(settingStatus))
        {
            settingStatus = WdfRegistryOpenKey(
                key,
                &subkeyName,
                KEY_READ,
                WDF_NO_OBJECT_ATTRIBUTES,
                &subkey);

            if (!NT_SUCCESS(settingStatus))
            {
                Trace(
                    TRACE_LEVEL_WARNING, 
                    TRACE_FLAG_WDFLOADING,
                    "Error opening registry subkey for 'Settings' - %!STATUS!", 
                    settingStatus);
            }
        }

        if (NT_SUCCESS(settingStatus))
        {

            settingStatus = WdfRegistryQueryULong(
                subkey,
                &connectInterruptName,
                &connectInterrupt);

            if (!NT_SUCCESS(settingStatus))
            {
                Trace(
                    TRACE_LEVEL_WARNING, 
                    TRACE_FLAG_WDFLOADING,
                    "Error querying registry value for 'ConnectInterrupt' - %!STATUS!", 
                    settingStatus);
            }
        }

        if (key != NULL)
        {
            WdfRegistryClose(key);
        }

        if (subkey != NULL)
        {
            WdfRegistryClose(subkey);
        }

        pDevice->ConnectInterrupt = (connectInterrupt == 1);
    }

exit:

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}
