/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    FireFly.h

Abstract:

    This is the header for the Windows Driver Framework version
    of the firefly filter driver.

Environment:

    Kernel mode

--*/

#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <initguid.h>
#include <wdmguid.h>

//
// Our drivers generated include from firefly.mof
// See makefile.inc for wmi commands
//
#include "fireflymof.h"

// Our drivers modules includes
#include "device.h"
#include "wmi.h"
#include "vfeature.h"
#include "magic.h"

//
// WDFDRIVER Object Events
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD FireFlyEvtDeviceAdd;

