/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    driver.h

Abstract:

    This module contains the function definitions for
    the WDF driver.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _DRIVER_H_
#define _DRIVER_H_

extern "C"

NTSTATUS
DriverEntry(
    _In_  PDRIVER_OBJECT   pDriverObject,
    _In_  PUNICODE_STRING  pRegistryPath
    );    

EVT_WDF_DRIVER_DEVICE_ADD       OnDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP  OnDriverCleanup;

#endif
