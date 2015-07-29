/*++

Copyright (c) Microsoft Corporation All Rights Reserved

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
#include <ntintsafe.h>
#include <initguid.h>
#include "driver.h"
#include "public.h"
#include <busenumMof.h>


#ifndef BUSENUM_H
#define BUSENUM_H

extern ULONG BusEnumDebugLevel;

#define BUS_TAG         'EsuB'
#define BUSRESOURCENAME L"MofResourceName"

#define DEF_STATICALLY_ENUMERATED_TOASTERS      0
#define MAX_STATICALLY_ENUMERATED_TOASTERS      10
#define MAX_INSTANCE_ID_LEN 80

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
// The goal of the identification and address description abstractions is that enough
// information is stored for a discovered device so that when it appears on the bus,
// the framework (with the help of the driver writer) can determine if it is a new or
// existing device.  The identification and address descriptions are opaque structures
// to the framework, they are private to the driver writer.  The only thing the framework
// knows about these descriptions is what their size is.
// The identification contains the bus specific information required to recognize
// an instance of a device on its the bus.  The identification information usually
// contains device IDs along with any serial or slot numbers.
// For some buses (like USB and PCI), the identification of the device is sufficient to
// address the device on the bus; in these instances there is no need for a separate
// address description.  Once reported, the identification description remains static
// for the lifetime of the device.  For example, the identification description that the
// PCI bus driver would use for a child would contain the vendor ID, device ID,
// subsystem ID, revision, and class for the device. This sample uses only identification
// description.
// On other busses (like 1394 and auto LUN SCSI), the device is assigned a dynamic
// address by the hardware (which may reassigned and updated periodically); in these
// instances the driver will use the address description to encapsulate this dynamic piece
// of data.    For example in a 1394 driver, the address description would contain the
// device's current generation count while the identification description would contain
// vendor name, model name, unit spec ID, and unit software version.
//
typedef struct _PDO_IDENTIFICATION_DESCRIPTION
{
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header; // should contain this header

    //
    // Unique serail number of the device on the bus
    //
    ULONG SerialNo;

    size_t CchHardwareIds;

    _Field_size_bytes_(CchHardwareIds) PWCHAR HardwareIds;

} PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;

//
// This is PDO device-extension.
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

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DEVICE_DATA, FdoGetData)

//
// Prototypes of functions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD Bus_EvtDeviceAdd;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL Bus_EvtIoDeviceControl;

EVT_WDF_CHILD_LIST_CREATE_DEVICE Bus_EvtDeviceListCreatePdo;
EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE Bus_EvtChildListIdentificationDescriptionCompare;
EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_CLEANUP Bus_EvtChildListIdentificationDescriptionCleanup;
EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_DUPLICATE Bus_EvtChildListIdentificationDescriptionDuplicate;

NTSTATUS
Bus_PlugInDevice(
    _In_ WDFDEVICE       Device,
    _In_ PWCHAR          HardwareIds,
    _In_ size_t          CchHardwareIds,
    _In_ ULONG           SerialNo
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
    _In_ WDFDEVICE       Device,
    _In_ PWDFDEVICE_INIT ChildInit,
    _In_reads_(MAX_INSTANCE_ID_LEN) PCWSTR HardwareIds,
    _In_ ULONG           SerialNo
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

