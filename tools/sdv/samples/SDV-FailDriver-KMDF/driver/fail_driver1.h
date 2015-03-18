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
#include <NTDDK.h>  
#include <wdf.h>
#ifdef __cplusplus
}
#endif

#include "fail_library1.h"

#ifdef __cplusplus
extern "C"
#endif
DRIVER_INITIALIZE DriverEntry; 

EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;

EVT_WDF_IO_QUEUE_IO_READ EvtIoRead;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

EVT_WDF_REQUEST_CANCEL  EvtRequestCancel;