/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.c

Abstract:

    Driver object related functionality for bthecho client device

Environment:

    Kernel mode only


--*/

#include "clisrv.h"
#include "driver.h"

#if defined(EVENT_TRACING)
//
// The trace message header (.tmh) file must be included in a source file
// before any WPP macro calls and after defining a WPP_CONTROL_GUIDS
// macro (defined in toaster.h). During the compilation, WPP scans the source
// files for DoTraceMessage() calls and builds a .tmh file which stores a unique
// data GUID for each message, the text resource string for each message,
// and the data types of the variables passed in for each message.  This file
// is automatically generated and used during post-processing.
//
#include "driver.tmh"
#else
#define _DRIVER_NAME_ "BthEchoSampleCli"
ULONG DebugLevel = TRACE_LEVEL_INFORMATION;
ULONG DebugFlag = 0xff;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, BthEchoCliEvtDriverCleanup)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
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
    NTSTATUS status;
    WDFDRIVER driver;
    WDF_OBJECT_ATTRIBUTES attributes;
        
    WDF_DRIVER_CONFIG DriverConfig;
    WDF_DRIVER_CONFIG_INIT(
                           &DriverConfig,
                           BthEchoCliEvtDriverDeviceAdd
                           );

    WPP_INIT_TRACING( DriverObject, RegistryPath );

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = BthEchoCliEvtDriverCleanup;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        NULL,
        &DriverConfig,
        &driver
        );

    if(!NT_SUCCESS(status))
    {
        WPP_CLEANUP(DriverObject);
    }

    return status;                            
}

VOID
BthEchoCliEvtDriverCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free resources allocated in DriverEntry that are automatically
    cleaned up framework.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE ();

    //
    // Driver remains unreferenced if EVENT_TRACING is not defined
    // in which case traces are sent to debugger instead of ETW
    //
    UNREFERENCED_PARAMETER(DriverObject);

    WPP_CLEANUP( WdfDriverWdmGetDriverObject( DriverObject ) );
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

    TraceEventsLevel - print level between 0 and 3, with 3 the most verbose

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

