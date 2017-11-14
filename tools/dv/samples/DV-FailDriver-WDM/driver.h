/*++
Copyright (c) 1990-2015    Microsoft Corporation All Rights Reserved

Module Name:

    driver.h

Abstract:

    This module contains the common declarations for the 
    bus, function and filter drivers.

Author:
Environment:

    kernel mode only

Notes:


Revision History:


--*/

//
// Define an Interface Guid to access the proprietary toaster interface.
// This guid is used to identify a specific interface in IRP_MN_QUERY_INTERFACE
// handler.
//

DEFINE_GUID(GUID_TOASTER_INTERFACE_STANDARD, 
        0xe0b27630, 0x5434, 0x11d3, 0xb8, 0x90, 0x0, 0xc0, 0x4f, 0xad, 0x51, 0x71);
// {E0B27630-5434-11d3-B890-00C04FAD5171}

//
// This guid is used in IoCreateDeviceSecure call to create PDOs. The idea is to
// allow the administrators to control access to the child device, in case the
// device gets enumerated as a raw device - no function driver, by modifying the 
// registry. If a function driver is loaded for the device, the system will override
// the security descriptor specified in the call to IoCreateDeviceSecure with the 
// one specifyied for the setup class of the child device.
//

DEFINE_GUID(GUID_SD_BUSENUM_PDO, 
        0x9d3039dd, 0xcca5, 0x4b4d, 0xb3, 0x3d, 0xe2, 0xdd, 0xc8, 0xa8, 0xc5, 0x2e);
// {9D3039DD-CCA5-4b4d-B33D-E2DDC8A8C52E}

//
// GUID definition are required to be outside of header inclusion pragma to avoid
// error during precompiled headers.
//

#ifndef __DRIVER_H
#define __DRIVER_H

//
// Define Interface reference/dereference routines for
//  Interfaces exported by IRP_MN_QUERY_INTERFACE
//

typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);

typedef
BOOLEAN
(*PTOASTER_GET_CRISPINESS_LEVEL)(
                           __in   PVOID Context,
                           __out  PUCHAR Level
                               );

typedef
BOOLEAN
(*PTOASTER_SET_CRISPINESS_LEVEL)(
                           __in   PVOID Context,
                           __out  UCHAR Level
                               );

typedef
BOOLEAN
(*PTOASTER_IS_CHILD_PROTECTED)(
                             __in PVOID Context
                             );

//
// Interface for getting and setting power level etc.,
//
typedef struct _TOASTER_INTERFACE_STANDARD {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    //
    // standard bus interfaces
    //
    PTOASTER_GET_CRISPINESS_LEVEL    GetCrispinessLevel;
    PTOASTER_SET_CRISPINESS_LEVEL    SetCrispinessLevel;
    PTOASTER_IS_CHILD_PROTECTED      IsSafetyLockEnabled; //):
} TOASTER_INTERFACE_STANDARD, *PTOASTER_INTERFACE_STANDARD;

#ifndef  STATUS_CONTINUE_COMPLETION //required to build driver in Win2K and XP build environment
//
// This value should be returned from completion routines to continue
// completing the IRP upwards. Otherwise, STATUS_MORE_PROCESSING_REQUIRED
// should be returned.
//
#define STATUS_CONTINUE_COMPLETION      STATUS_SUCCESS

#endif

#endif



