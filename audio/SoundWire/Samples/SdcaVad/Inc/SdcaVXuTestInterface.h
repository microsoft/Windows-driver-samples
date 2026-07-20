/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    SdcaVXuTestInterface.h

Abstract:

    Contains definitions to allow user mode applications to communicate
    directly with private interface of SdcaVXu driver

Environment:

    Kernel mode, User mode

--*/

#pragma once

// The SDCA XU Driver can add a device interface to any of the raw PDOs it creates.
// 
// We'll add the GUID_DEVINTERFACE_SDCAVXU_TEST_RAWCONTROL to the initial raw PDO
// the XU driver creates (which currently isn't used for any other purpose).
//
// User-mode applications can then use this device interface to find the target
// for IOCTL requests that are intended for the XU driver.
//
// A real XU driver could add a device interface to the initial raw PDO or to
// any of the Circuit Devices it creates to support the XU driver circuits, or
// it could use WdfControlDeviceInitAllocate to create a Control Device Object
// to accept IOCTL requests from user mode (though a Control Device Object
// cannot be used with WdfDeviceCreateDeviceInterface and would instead be
// used with WdfDeviceCreateSymbolicLink)
//
// A real XU driver could also use WdfDeviceInitAssignName to enable user-mode
// applications to call CreateFile for the device.

// {48124666-FA50-47DE-A72D-0833510DBF96}
DEFINE_GUID(GUID_DEVINTERFACE_SDCAVXU_TEST_RAWCONTROL,
0x48124666, 0xfa50, 0x47de, 0xa7, 0x2d, 0x8, 0x33, 0x51, 0xd, 0xbf, 0x96);

typedef struct _SDCAVXU_TEST_DATA
{
    ULONG Data;
} SDCAVXU_TEST_DATA, *PSDCAVXU_TEST_DATA;

#define IOCTL_SDCAVXU_INTERFACE_TEST \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1, METHOD_BUFFERED, FILE_ANY_ACCESS)
