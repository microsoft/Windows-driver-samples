/*++

Copyright (c) 2003 Microsoft Corporation All Rights Reserved

Module Name:

    busenum.h

Abstract:

    This module contains the common private declarations
    for the Toaster Bus enumerator.

Environment:

    kernel mode only

--*/

#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <initguid.h>
#include "driver.h"
#include "public.h"
#include <busenumMof.h>

#ifndef BUSENUM_H
#define BUSENUM_H

extern ULONG BusEnumDebugLevel;

#define BUSRESOURCENAME L"MofResourceName"

#define DEF_STATICALLY_ENUMERATED_TOASTERS      0
#define MAX_STATICALLY_ENUMERATED_TOASTERS      10

#ifndef min
#define min(_a, _b)     (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define max(_a, _b)     (((_a) > (_b)) ? (_a) : (_b))
#endif

//
// Structure for reporting data to WMI
//

typedef ToasterBusInformation TOASTER_BUS_WMI_STD_DATA, * PTOASTER_BUS_WMI_STD_DATA;

//
// The device extension for the PDOs.
// That's of the toaster device which this bus driver enumerates.
//

typedef struct _PDO_DEVICE_DATA
{
    // Unique serail number of the device on the bus

    ULONG SerialNo;

} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_DEVICE_DATA, PdoGetData)

//
// The device extension of the bus itself.  From whence the PDO's are born.
//

typedef struct _FDO_DEVICE_DATA
{
    TOASTER_BUS_WMI_STD_DATA   StdToasterBusData;

    WDFWAITLOCK ChildLock;

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DEVICE_DATA, FdoGetData)

typedef struct _QUEUE_DATA
{
    PFDO_DEVICE_DATA FdoData;

} QUEUE_DATA, *PQUEUE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_DATA, QueueGetData)

//
// Prototypes of functions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD Bus_EvtDeviceAdd;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL Bus_EvtIoDeviceControl;

NTSTATUS
Bus_PlugInDevice(
    _In_ WDFDEVICE  Device,
    _In_ PWSTR      HardwareIds,
    _In_ ULONG      SerialNo
    );

NTSTATUS
Bus_UnPlugDevice(
    WDFDEVICE   Device,
    ULONG       SerialNo
    );


NTSTATUS
Bus_EjectDevice(
    WDFDEVICE   Device,
    ULONG       SerialNo
    );

NTSTATUS
Bus_CreatePdo(
    _In_ WDFDEVICE  Device,
    _In_ PWSTR      HardwareIds,
    _In_ ULONG      SerialNo
    );


NTSTATUS
Bus_DoStaticEnumeration(
    IN WDFDEVICE Device
    );

//
// Interface functions
//

BOOLEAN
Bus_GetCrispinessLevel(
    IN   WDFDEVICE ChildDevice,
    OUT  PUCHAR Level
    );

BOOLEAN
Bus_SetCrispinessLevel(
    IN   WDFDEVICE ChildDevice,
    OUT  UCHAR Level
    );

BOOLEAN
Bus_IsSafetyLockEnabled(
    IN PVOID Context
    );

//
// Defined in wmi.c
//

NTSTATUS
Bus_WmiRegistration(
    WDFDEVICE      Device
    );

EVT_WDF_WMI_INSTANCE_SET_ITEM Bus_EvtStdDataSetItem;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE Bus_EvtStdDataSetInstance;
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE Bus_EvtStdDataQueryInstance;

#endif

