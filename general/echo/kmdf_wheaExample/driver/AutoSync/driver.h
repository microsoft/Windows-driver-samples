/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    driver.h

Abstract:

    This is a C version of a very simple sample driver that illustrates
    how to use the driver framework and demonstrates best practices.

--*/

#define INITGUID

#include <ntddk.h>
#include <wdf.h>

#include "device.h"
#include "queue.h"

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EchoEvtDeviceAdd;

NTSTATUS
EchoPrintDriverVersion(
    );

