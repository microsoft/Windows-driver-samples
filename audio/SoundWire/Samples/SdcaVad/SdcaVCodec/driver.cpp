/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Driver.cpp

Abstract:

    Sample soundwire Codec driver

Environment:

    Kernel mode only

--*/

#include "private.h"

#ifndef __INTELLISENSE__
#include "driver.tmh"
#endif

RECORDER_LOG g_SDCAVCodecLog{ nullptr };

INIT_CODE_SEG
void
Test_ClientVersionHigherThanFramework()
{
    PAGED_CODE();
    
    // example on how to check if a function is available.
    /*
    if (ACX_IS_FUNCTION_AVAILABLE(AcxCircuitCreate)) {
        DbgPrint("Available: AcxCircuitCreate\n");
    }
    else
    {
        DbgPrint("Not available: AcxCircuitCreate\n");
        ASSERT(FALSE);
    }
    */
    
    if (ACX_IS_FIELD_AVAILABLE(ACX_DEVICEINIT_CONFIG, SynchronizationScope)) {
        ACX_DEVICEINIT_CONFIG config;
        ACX_DEVICEINIT_CONFIG_INIT(&config);
        DbgPrint("Available: ACX_DEVICEINIT_CONFIG.SynchronizationScope\n");
    }
    else
    {
        DbgPrint("Not available: ACX_DEVICEINIT_CONFIG.SynchronizationScope\n");
        ASSERT(FALSE);
    }
}

PAGED_CODE_SEG
VOID Codec_DriverUnload(_In_ WDFDRIVER Driver)
{
    PAGED_CODE(); 

    if (!Driver)
    {
        ASSERT(FALSE);
        return;
    }

    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));

    if (g_RegistryPath.Buffer != NULL)
    {
        ExFreePool(g_RegistryPath.Buffer);
        RtlZeroMemory(&g_RegistryPath, sizeof(g_RegistryPath));
    }

    return;
}

INIT_CODE_SEG
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    PAGED_CODE(); 
    
    NTSTATUS status = STATUS_SUCCESS;

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    auto exit = scope_exit([&status, &DriverObject]() {
        if (!NT_SUCCESS(status))
        {
            if (g_RegistryPath.Buffer != NULL)
            {
                ExFreePool(g_RegistryPath.Buffer);
                RtlZeroMemory(&g_RegistryPath, sizeof(g_RegistryPath));
            }

            WPP_CLEANUP(DriverObject);
        }
        else
        {
            DrvLogInfo(g_SDCAVCodecLog, FLAG_INIT, "ACX SDCA Virtual Codec Driver Init complete, %!STATUS!", status);
        }
        });

    RETURN_NTSTATUS_IF_FAILED(CopyRegistrySettingsPath(RegistryPath));

    //
    // Initiialize driver config to control the attributes that
    // are global to the driver. Note that framework by default
    // provides a driver unload routine. If you create any resources
    // in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by manually setting the EvtDriverUnload in the
    // config structure. In general xxx_CONFIG_INIT macros are provided to
    // initialize most commonly used members.
    //

    WDF_DRIVER_CONFIG wdfCfg;
    WDF_DRIVER_CONFIG_INIT(&wdfCfg, Codec_EvtBusDeviceAdd);
    wdfCfg.EvtDriverUnload = Codec_DriverUnload;
    
    //
    // Add a driver context. (for illustration purposes only).
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_DRIVER_CONTEXT);

    //
    // Create a framework driver object to represent our driver.
    //
    WDFDRIVER driver;
    RETURN_NTSTATUS_IF_FAILED(WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes,            // Driver Attributes
        &wdfCfg,                // Driver Config Info
        &driver                 // hDriver
        ));

    RECORDER_CONFIGURE_PARAMS recorderConfig;
    RECORDER_CONFIGURE_PARAMS_INIT(&recorderConfig);
    recorderConfig.CreateDefaultLog = FALSE;
    WppRecorderConfigure(&recorderConfig); 

    RECORDER_LOG_CREATE_PARAMS recorderLogCreateParams;
    RECORDER_LOG_CREATE_PARAMS_INIT(&recorderLogCreateParams, NULL);
    recorderLogCreateParams.TotalBufferSize = WPP_TOTAL_BUFFER_SIZE;
    recorderLogCreateParams.ErrorPartitionSize = WPP_ERROR_PARTITION_SIZE;

    RtlStringCbPrintfA(recorderLogCreateParams.LogIdentifier,
        RECORDER_LOG_IDENTIFIER_MAX_CHARS,
        "SDCAVCodec");

    RECORDER_LOG logHandle = NULL;
    status = WppRecorderLogCreate(&recorderLogCreateParams, &logHandle);
    if (!NT_SUCCESS(status))
    {
        logHandle = NULL;

        // Non fatal failure
        status = STATUS_SUCCESS;
    }

    g_SDCAVCodecLog = logHandle;
    
    //
    // Post init.
    //
    ACX_DRIVER_CONFIG acxCfg;
    ACX_DRIVER_CONFIG_INIT(&acxCfg);
    
    RETURN_NTSTATUS_IF_FAILED(AcxDriverInitialize(driver, &acxCfg));

    //
    // Test ACX bindings.
    //
    ACX_DRIVER_VERSION_AVAILABLE_PARAMS ver;
    ACX_DRIVER_VERSION_AVAILABLE_PARAMS_INIT(&ver, 1, 0);
    if (!AcxDriverIsVersionAvailable(driver, &ver)) {
        status = STATUS_DRIVER_INTERNAL_ERROR;
        DbgPrint("Unexpected ACX library version.\n");
        ASSERT(FALSE);
    }
    RETURN_NTSTATUS_IF_FAILED(status);
    
    Test_ClientVersionHigherThanFramework();
    
    return status;
}


