/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Monitor Sample driver initialization routines

Environment:

    Kernel mode

--*/

#include <ndis.h>
#include <ntddk.h>
#include <wdf.h>

#include <fwpmk.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

#include <fwpsk.h>

#pragma warning(pop)

#include "ioctl.h"

#include "msnmntr.h"
#include "ctl.h"

#include "notify.h"

//
// Software Tracing Definitions 
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(MsnMntrInit,(e7db16bb, 41be, 4c05, b73e, 5feca06f8207),  \
        WPP_DEFINE_BIT(TRACE_INIT)               \
        WPP_DEFINE_BIT(TRACE_SHUTDOWN) )

#include "init.tmh"

DEVICE_OBJECT* gWdmDevice;

// ===========================================================================
//
// LOCAL PROTOTYPES
//
// ===========================================================================

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD MonitorEvtDriverUnload;

// We're using what looks like a EVT_WDF_DRIVER_DEVICE_ADD callback, to keep
// this looking like a normal KMDF driver.  However, since this is a non-pnp
// driver, it will not be used as a callback; we will call it ourselves at the
// end of DriverEntry.  So, do not declare it as a callback.
// The NONPNP sample demonstrates this as well.
NTSTATUS
MonitorEvtDeviceAdd(
   _In_ PWDFDEVICE_INIT pInit
   );

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

NTSTATUS
DriverEntry(
    _In_ DRIVER_OBJECT* driverObject,
    _In_ UNICODE_STRING* registryPath
    )
/*++

Routine Description:

    Main driver entry point. Called at driver load time

Arguments:

    driverObject            Our driver
    registryPath            A reg key where we can keep parameters

Return Value:

    status of our initialization. A status != STATUS_SUCCESS aborts the
    driver load and we don't get called again.

    Each component is responsible for logging any error that causes the
    driver load to fail.

--*/
{
   NTSTATUS status;
   WDF_DRIVER_CONFIG config;
   WDFDRIVER driver;
   PWDFDEVICE_INIT pInit = NULL;

   // Request NX Non-Paged Pool when available
   ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

   //
   // This macro is required to initialize software tracing on XP and beyond
   // For XP and beyond use the DriverObject as the first argument.
   // 
   
   WPP_INIT_TRACING(driverObject,registryPath);

   DoTraceMessage(TRACE_INIT, "Initializing MonitorSample Driver");

   WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
   config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
   config.EvtDriverUnload = MonitorEvtDriverUnload;

   status = WdfDriverCreate(
               driverObject,
               registryPath,
               WDF_NO_OBJECT_ATTRIBUTES,
               &config,
               &driver
               );

   if (!NT_SUCCESS(status))
   {
      goto cleanup;
   }

   pInit = WdfControlDeviceInitAllocate(driver, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);

   if (!pInit)
   {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto cleanup;
   }
   
   status = MonitorEvtDeviceAdd(pInit);

cleanup:
   if (!NT_SUCCESS(status))
   {
      DoTraceMessage(TRACE_INIT, "MonitorSample Initialization Failed.");

      WPP_CLEANUP(driverObject);
   }

   return status;
}

NTSTATUS
MonitorEvtDeviceAdd(
   _In_ PWDFDEVICE_INIT pInit
   )
{
   NTSTATUS status;
   WDFDEVICE device;
   DECLARE_CONST_UNICODE_STRING(ntDeviceName, MONITOR_DEVICE_NAME);
   DECLARE_CONST_UNICODE_STRING(symbolicName, MONITOR_SYMBOLIC_NAME);

   WdfDeviceInitSetDeviceType(pInit, FILE_DEVICE_NETWORK);
   WdfDeviceInitSetCharacteristics(pInit, FILE_DEVICE_SECURE_OPEN, FALSE);
   status = WdfDeviceInitAssignName(pInit, &ntDeviceName);
   if (!NT_SUCCESS(status))
   {
      goto cleanup;
   }

   status = WdfDeviceCreate(&pInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
   if (!NT_SUCCESS(status))
   {
      goto cleanup;
   }

   status = WdfDeviceCreateSymbolicLink(device, &symbolicName);
   if (!NT_SUCCESS(status))
   {
      goto cleanup;
   }

   status = MonitorCtlDriverInit(&device);
   if (!NT_SUCCESS(status))
   {
      goto cleanup;
   }

   gWdmDevice = WdfDeviceWdmGetDeviceObject(device);
   status = MonitorCoInitialize(gWdmDevice);
   if (!NT_SUCCESS(status))
   {
      goto cleanup;
   }

   status = MonitorNfInitialize(gWdmDevice);
   if (!NT_SUCCESS(status))
   {
      goto cleanup;
   }
   
   WdfControlFinishInitializing(device);

cleanup:
   // If WdfDeviceCreate was successful, it will set pInit to NULL.
   if (pInit)
   {
      WdfDeviceInitFree(pInit);
   }

   return status;
}

void
MonitorEvtDriverUnload(
   _In_ WDFDRIVER Driver
   )
/*++

Routine Description:

    Called to indicate that we are being unloaded and to cause an orderly
    shutdown

Arguments:

    driverObject            Our driver

Return Value:

    None

--*/
{
   DRIVER_OBJECT* driverObject;

   MonitorCoUninitialize();
   MonitorNfUninitialize();

   DoTraceMessage(TRACE_SHUTDOWN, "MonitorSample Driver Shutting Down");

   driverObject = WdfDriverWdmGetDriverObject(Driver);
   WPP_CLEANUP(driverObject);
}
