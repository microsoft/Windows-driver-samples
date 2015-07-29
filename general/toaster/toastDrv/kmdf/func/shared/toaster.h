/*++

Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved

Module Name:

    Toaster.h

Abstract:

    Header file for the toaster driver modules.

Environment:

    Kernel mode

--*/


#if !defined(_TOASTER_H_)
#define _TOASTER_H_

#include <ntddk.h>
#include <wdf.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include "wmilib.h"
#include <initguid.h>
#include "..\inc\driver.h"
#include "..\inc\public.h"

// For Featured driver only
#ifdef TOASTER_FUNC_FEATURED
#include <wpprecorder.h>
#endif // TOASTER_FUNC_FEATURED

#define TOASTER_POOL_TAG (ULONG) 'saoT'

#define MOFRESOURCENAME L"ToasterWMI"

#define TOASTER_FUNC_DEVICE_LOG_ID "ToasterDevice"
//
// The device extension for the device object
//
typedef struct _FDO_DATA
{

    WDFWMIINSTANCE WmiDeviceArrivalEvent;

    BOOLEAN     WmiPowerDeviceEnableRegistered;

    TOASTER_INTERFACE_STANDARD BusInterface;

// For Featured driver only
#ifdef TOASTER_FUNC_FEATURED
    RECORDER_LOG    WppRecorderLog;
#endif // TOASTER_FUNC_FEATURED

}  FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, ToasterFdoGetData)


//
// Connector Types
//

#define TOASTER_WMI_STD_I8042 0
#define TOASTER_WMI_STD_SERIAL 1
#define TOASTER_WMI_STD_PARALEL 2
#define TOASTER_WMI_STD_USB 3

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD ToasterEvtDriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD ToasterEvtDeviceAdd;

EVT_WDF_DEVICE_CONTEXT_CLEANUP ToasterEvtDeviceContextCleanup;
EVT_WDF_DEVICE_D0_ENTRY ToasterEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT ToasterEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE ToasterEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE ToasterEvtDeviceReleaseHardware;

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT ToasterEvtDeviceSelfManagedIoInit;

//
// Io events callbacks.
//
EVT_WDF_IO_QUEUE_IO_READ ToasterEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE ToasterEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL ToasterEvtIoDeviceControl;
EVT_WDF_DEVICE_FILE_CREATE ToasterEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE ToasterEvtFileClose;

NTSTATUS
ToasterWmiRegistration(
    _In_ WDFDEVICE Device
    );

//
// Power events callbacks
//
EVT_WDF_DEVICE_ARM_WAKE_FROM_S0 ToasterEvtDeviceArmWakeFromS0;
EVT_WDF_DEVICE_ARM_WAKE_FROM_SX ToasterEvtDeviceArmWakeFromSx;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0 ToasterEvtDeviceDisarmWakeFromS0;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX ToasterEvtDeviceDisarmWakeFromSx;
EVT_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED ToasterEvtDeviceWakeFromS0Triggered;
EVT_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED ToasterEvtDeviceWakeFromSxTriggered;

PCHAR
DbgDevicePowerString(
    IN WDF_POWER_DEVICE_STATE Type
    );

//
// WMI event callbacks
//
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceStdDeviceDataQueryInstance;
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceToasterControlQueryInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceStdDeviceDataSetInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceToasterControlSetInstance;
EVT_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceToasterControlSetItem;
EVT_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceStdDeviceDataSetItem;
EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD EvtWmiInstanceToasterControlExecuteMethod;

NTSTATUS
ToasterFireArrivalEvent(
    _In_ WDFDEVICE  Device
    );

#endif  // _TOASTER_H_

