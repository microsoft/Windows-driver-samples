/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    driver.cpp

Abstract:

    This module contains the WDF driver initialization 
    functions for the controller driver.

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
    driverConfig.DriverPoolTag = SI2C_POOL_TAG;

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
        "Created WDFDRIVER %p",
        fxDriver);

exit:

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

VOID
OnDriverCleanup(
    _In_ WDFOBJECT Object
    )
{
    UNREFERENCED_PARAMETER(Object);

    WPP_CLEANUP(NULL);
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

    PPBC_DEVICE pDevice;
    NTSTATUS status;
    
    UNREFERENCED_PARAMETER(FxDriver);

    //
    // Configure DeviceInit structure
    //
    
    status = SpbDeviceInitConfig(FxDeviceInit);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_WDFLOADING,
            "Failed SpbDeviceInitConfig() for WDFDEVICE_INIT %p - %!STATUS!", 
            FxDeviceInit,
            status);

        goto exit;
    }  
        
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
        pnpCallbacks.EvtDeviceSelfManagedIoInit = OnSelfManagedIoInit;
        pnpCallbacks.EvtDeviceSelfManagedIoCleanup = OnSelfManagedIoCleanup;

        WdfDeviceInitSetPnpPowerEventCallbacks(FxDeviceInit, &pnpCallbacks);
    }

    //
    // Note: The SPB class extension sets a default 
    //       security descriptor to allow access to 
    //       user-mode drivers. This can be overridden 
    //       by calling WdfDeviceInitAssignSDDLString()
    //       with the desired setting. This must be done
    //       after calling SpbDeviceInitConfig() but
    //       before WdfDeviceCreate().
    //
    

    //
    // Create the device.
    //

    {
        WDF_OBJECT_ATTRIBUTES deviceAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, PBC_DEVICE);
        WDFDEVICE fxDevice;

        status = WdfDeviceCreate(
            &FxDeviceInit, 
            &deviceAttributes,
            &fxDevice);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Failed to create WDFDEVICE from WDFDEVICE_INIT %p - %!STATUS!", 
                FxDeviceInit,
                status);

            goto exit;
        }

        pDevice = GetDeviceContext(fxDevice);
        NT_ASSERT(pDevice != NULL);

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
    // Bind a SPB controller object to the device.
    //

    {
        SPB_CONTROLLER_CONFIG spbConfig;
        SPB_CONTROLLER_CONFIG_INIT(&spbConfig);

        //
        // Register for target connect callback.  The driver
        // does not need to respond to target disconnect.
        //

        spbConfig.EvtSpbTargetConnect    = OnTargetConnect;

        //
        // Register for IO callbacks.
        //

        spbConfig.ControllerDispatchType = WdfIoQueueDispatchSequential;
        spbConfig.PowerManaged           = WdfTrue;
        spbConfig.EvtSpbIoRead           = OnRead;
        spbConfig.EvtSpbIoWrite          = OnWrite;
        spbConfig.EvtSpbIoSequence       = OnSequence;
        spbConfig.EvtSpbControllerLock   = OnControllerLock;
        spbConfig.EvtSpbControllerUnlock = OnControllerUnlock;

        status = SpbDeviceInitialize(pDevice->FxDevice, &spbConfig);
       
        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Failed SpbDeviceInitialize() for WDFDEVICE %p - %!STATUS!", 
                pDevice->FxDevice,
                status);

            goto exit;
        }

        //
        // Register for IO other callbacks.
        //

        SpbControllerSetIoOtherCallback(
            pDevice->FxDevice,
            OnOther,
            OnOtherInCallerContext);
    }

    //
    // Set target object attributes.
    //

    {
        WDF_OBJECT_ATTRIBUTES targetAttributes; 
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&targetAttributes, PBC_TARGET);

        SpbControllerSetTargetAttributes(pDevice->FxDevice, &targetAttributes);
    }

    //
    // Set request object attributes.
    //

    {
        WDF_OBJECT_ATTRIBUTES requestAttributes; 
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, PBC_REQUEST);
        
        //
        // NOTE: Be mindful when registering for EvtCleanupCallback or 
        //       EvtDestroyCallback. IO requests arriving in the class
        //       extension, but not presented to the driver (due to
        //       cancellation), will still have their cleanup and destroy 
        //       callbacks invoked.
        //

        SpbControllerSetRequestAttributes(pDevice->FxDevice, &requestAttributes);
    }

    //
    // Create an interrupt object, interrupt spinlock,
    // and register callbacks.
    //

    {            
        //
        // Create the interrupt spinlock.
        //

        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = pDevice->FxDevice;
        
        WDFSPINLOCK interruptLock;

        status = WdfSpinLockCreate(
           &attributes,
           &interruptLock);
        
        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING, 
                "Failed to create interrupt spinlock for WDFDEVICE %p - %!STATUS!", 
                pDevice->FxDevice,
                status);

            goto exit;
        }

        //
        // Create the interrupt object.
        //

        WDF_INTERRUPT_CONFIG interruptConfig;

        WDF_INTERRUPT_CONFIG_INIT(
            &interruptConfig,
            OnInterruptIsr,
            OnInterruptDpc);

        interruptConfig.SpinLock = interruptLock;

        status = WdfInterruptCreate(
            pDevice->FxDevice,
            &interruptConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &pDevice->InterruptObject);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Failed to create interrupt object for WDFDEVICE %p - %!STATUS!",
                pDevice->FxDevice,
                status);

            goto exit;
        }
    }
    
    //
    // Create the delay timer to stall between transfers.
    //
    {    
        WDF_TIMER_CONFIG      wdfTimerConfig;
        WDF_OBJECT_ATTRIBUTES timerAttributes;

        WDF_TIMER_CONFIG_INIT(&wdfTimerConfig, OnDelayTimerExpired);
        WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
        timerAttributes.ParentObject = pDevice->FxDevice;

        status = WdfTimerCreate(
            &wdfTimerConfig,
            &timerAttributes,
            &(pDevice->DelayTimer)
            );

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING, 
                "Failed to create delay timer for WDFDEVICE %p - %!STATUS!", 
                pDevice->FxDevice,
                status);

            goto exit;
        }
    }

    //
    // Create the spin lock to synchronize access
    // to the controller driver.
    //
    
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = pDevice->FxDevice;

    status = WdfSpinLockCreate(
       &attributes,
       &pDevice->Lock);
    
    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_WDFLOADING, 
            "Failed to create device spinlock for WDFDEVICE %p - %!STATUS!", 
            pDevice->FxDevice,
            status);

        goto exit;
    }
    
    //
    // Configure idle settings to use system
    // managed idle timeout.
    //
    {    
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(
            &idleSettings, 
            IdleCannotWakeFromS0);

        //
        // Explicitly set initial idle timeout delay.
        //

        idleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;
        idleSettings.IdleTimeout = IDLE_TIMEOUT_MONITOR_ON;

        status = WdfDeviceAssignS0IdleSettings(
            pDevice->FxDevice, 
            &idleSettings);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_WDFLOADING,
                "Failed to initalize S0 idle settings for WDFDEVICE %p- %!STATUS!",
                pDevice->FxDevice,
                status);
                
            goto exit;
        }
    }

exit:

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}
