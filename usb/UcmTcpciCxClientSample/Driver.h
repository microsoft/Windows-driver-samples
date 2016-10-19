/*++

Module Name:

    Driver.h

Abstract:

    This file contains the driver declarations.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include <wdmguid.h>
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>
#include <spb.h>
#include <UcmTcpciCx.h>

#include "I2C.h"
#include "Device.h"
#include "Alert.h"
#include "PortControllerInterface.h"
#include "Queue.h"
#include "Register.h"
#include "Trace.h"

EXTERN_C_START

DRIVER_INITIALIZE
DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD
EvtDeviceAdd;

EVT_WDF_OBJECT_CONTEXT_CLEANUP
EvtDriverContextCleanup;

EXTERN_C_END
