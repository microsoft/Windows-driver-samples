/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fail_driver1.h

Environment:

    Kernel mode

--*/

#ifdef __cplusplus
extern "C" {
#endif
#include <wdm.h>
#ifdef __cplusplus
}
#endif
// suppress these warning for this fail_driver
// SDV can find defects without annotations
#pragma warning(disable:6387)
#pragma warning(disable:28166)
#pragma warning(disable:28165)
#pragma warning(disable:28121)
#pragma warning(disable:28150)
#pragma warning(disable:28160)
#pragma warning(disable:26135)
#pragma warning(disable:26165)
#pragma warning(disable:28930)
#pragma warning(disable:28931)

typedef struct _DRIVER_DEVICE_EXTENSION
{
    PKSPIN_LOCK queueLock; 
    PRKEVENT  Event;
    KPRIORITY  Increment;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    ULONG ControllerVector;  
    PKINTERRUPT InterruptObject;
}
DRIVER_DEVICE_EXTENSION,*PDRIVER_DEVICE_EXTENSION;

#ifdef __cplusplus
extern "C"
#endif
DRIVER_INITIALIZE DriverEntry;

DRIVER_ADD_DEVICE DriverAddDevice;

_Dispatch_type_(IRP_MJ_CREATE)
DRIVER_DISPATCH DispatchCreate;

_Dispatch_type_(IRP_MJ_READ)
DRIVER_DISPATCH DispatchRead;

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH DispatchPower;

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
DRIVER_DISPATCH DispatchSystemControl;

_Dispatch_type_(IRP_MJ_PNP)
DRIVER_DISPATCH DispatchPnp;

IO_COMPLETION_ROUTINE CompletionRoutine;

KSERVICE_ROUTINE InterruptServiceRoutine;

IO_DPC_ROUTINE DpcForIsrRoutine;

DRIVER_UNLOAD DriverUnload;
