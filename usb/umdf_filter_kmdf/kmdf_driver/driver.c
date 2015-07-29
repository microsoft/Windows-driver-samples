/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.c

Abstract:

    Main module.

    This driver is for Open System Resources USB-FX2 Learning Kit designed
    and built by OSR specifically for use in teaching software developers how to write
    drivers for USB devices.

    The board supports a single configuration. The board automatically
    detects the speed of the host controller, and supplies either the
    high or full speed configuration based on the host controller's speed.

    The firmware supports 3 endpoints:

    Endpoint number 1 is used to indicate the state of the 8-switch
    switch-pack on the OSR USB-FX2 board. A single byte representing
    the switch state is sent (a) when the board is first started,
    (b) when the board resumes after selective-suspend,
    (c) whenever the state of the switches is changed.

    Endpoints 6 and 8 perform an internal loop-back function.
    Data that is sent to the board at EP6 is returned to the host on EP8.

    For further information on the endpoints, please refer to the spec
    http://www.osronline.com/hardware/OSRFX2_32.pdf.

    Vendor ID of the device is 0x4705 and Product ID is 0x210.

Environment:

    Kernel mode only

--*/

#include <osrusbfx2.h>

#if defined(EVENT_TRACING)
//
// The trace message header (.tmh) file must be included in a source file
// before any WPP macro calls and after defining a WPP_CONTROL_GUIDS
// macro (defined in trace.h). During the compilation, WPP scans the source
// files for DoTraceMessage() calls and builds a .tmh file which stores a unique
// data GUID for each message, the text resource string for each message,
// and the data types of the variables passed in for each message.  This file
// is automatically generated and used during post-processing.
//
#include "driver.tmh"
#else
ULONG DebugLevel = TRACE_LEVEL_INFORMATION;
ULONG DebugFlag = 0xff;
#endif

PFN_IO_GET_ACTIVITY_ID_IRP g_pIoGetActivityIdIrp;
PFN_IO_SET_DEVICE_INTERFACE_PROPERTY_DATA g_pIoSetDeviceInterfacePropertyData;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, OsrFxEvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
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
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
{
    WDF_DRIVER_CONFIG       config;
    NTSTATUS                status;
    WDF_OBJECT_ATTRIBUTES   attributes;
    UNICODE_STRING          funcName;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING( DriverObject, RegistryPath );

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                       "OSRUSBFX2 Driver Sample - Driver Framework Edition.\n");

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                "Built %s %s\n", __DATE__, __TIME__);

    //
    // IRP activity ID functions are available on some versions, save them into
    // globals (or NULL if not available)
    //
    RtlInitUnicodeString(&funcName, L"IoGetActivityIdIrp");
    g_pIoGetActivityIdIrp = (PFN_IO_GET_ACTIVITY_ID_IRP) (ULONG_PTR)
        MmGetSystemRoutineAddress(&funcName);

    //
    // The Device interface property set is available on some version, save it
    // into globals (or NULL if not available)
    //
    RtlInitUnicodeString(&funcName, L"IoSetDeviceInterfacePropertyData");
    g_pIoSetDeviceInterfacePropertyData = (PFN_IO_SET_DEVICE_INTERFACE_PROPERTY_DATA) (ULONG_PTR)
        MmGetSystemRoutineAddress(&funcName);

    //
    // Register with ETW (unified tracing)
    // 
    EventRegisterOSRUSBFX2();

    //
    // Initiialize driver config to control the attributes that
    // are global to the driver. Note that framework by default
    // provides a driver unload routine. If you create any resources
    // in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by manually setting the EvtDriverUnload in the
    // config structure. In general xxx_CONFIG_INIT macros are provided to
    // initialize most commonly used members.
    //

    WDF_DRIVER_CONFIG_INIT(
        &config,
        OsrFxEvtDeviceAdd
        );

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = OsrFxEvtDriverContextCleanup;

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes, // Driver Object Attributes
        &config,          // Driver Config Info
        WDF_NO_HANDLE // hDriver
        );

    if (!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                "WdfDriverCreate failed with status 0x%x\n", status);
        //
        // Cleanup tracing here because DriverContextCleanup will not be called
        // as we have failed to create WDFDRIVER object itself.
        // Please note that if your return failure from DriverEntry after the
        // WDFDRIVER object is created successfully, you don't have to
        // call WPP cleanup because in those cases DriverContextCleanup
        // will be executed when the framework deletes the DriverObject.
        //
        WPP_CLEANUP(DriverObject);
        EventUnregisterOSRUSBFX2();
    }

    return status;
}

VOID
OsrFxEvtDriverContextCleanup(
    WDFOBJECT Driver
    )
/*++
Routine Description:

    Free resources allocated in DriverEntry that are not automatically
    cleaned up by the framework.

Arguments:

    Driver - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                    "--> OsrFxEvtDriverContextCleanup\n");

    WPP_CLEANUP( WdfDriverWdmGetDriverObject( (WDFDRIVER)Driver ));

    UNREFERENCED_PARAMETER(Driver); // For the case when WPP is not being used.
	
    EventUnregisterOSRUSBFX2();
}

#if !defined(EVENT_TRACING)

VOID
TraceEvents (
    _In_ ULONG DebugPrintLevel,
    _In_ ULONG DebugPrintFlag,
    _Printf_format_string_
    _In_ PCSTR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for the sample driver.

Arguments:

    DebugPrintLevel - print level between 0 and 3, with 3 the most verbose
    DebugPrintFlag - message mask
    DebugMessage - format string of the message to print
    ... - values used by the format string

Return Value:

    None.

 --*/
 {
#if DBG
#define     TEMP_BUFFER_SIZE        1024
    va_list    list;
    CHAR       debugMessageBuffer[TEMP_BUFFER_SIZE];
    NTSTATUS   status;

    va_start(list, DebugMessage);

    if (DebugMessage) {

        //
        // Using new safe string functions instead of _vsnprintf.
        // This function takes care of NULL terminating if the message
        // is longer than the buffer.
        //
        status = RtlStringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
                                      DebugMessage,
                                      list );
        if(!NT_SUCCESS(status)) {

            DbgPrint (_DRIVER_NAME_": RtlStringCbVPrintfA failed 0x%x\n", status);
            return;
        }
        if (DebugPrintLevel <= TRACE_LEVEL_ERROR ||
            (DebugPrintLevel <= DebugLevel &&
             ((DebugPrintFlag & DebugFlag) == DebugPrintFlag))) {
            DbgPrint("%s %s", _DRIVER_NAME_, debugMessageBuffer);
        }
    }
    va_end(list);

    return;
#else
    UNREFERENCED_PARAMETER(DebugPrintLevel);
    UNREFERENCED_PARAMETER(DebugPrintFlag);
    UNREFERENCED_PARAMETER(DebugMessage);
#endif
}

#endif




