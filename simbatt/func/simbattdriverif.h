/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simbattdriverif.h

Abstract:

    This module contains the interfaces used to communicate between usermode
    and the simulated battery driver stack.

    N.B. This code is provided "AS IS" without any expressed or implied warranty.

--*/

//---------------------------------------------------------------------- Pragmas

#pragma once

//--------------------------------------------------------------------- Includes

#include <initguid.h>

//------------------------------------------------------------------ Definitions

//
// Battery bus driver interface
//

// {780AC894-01FF-4b5e-B4C8-9C00709200EB}
DEFINE_GUID(BATTBUS_DEVINTERFACE_GUID,
    0x780ac894, 0x1ff, 0x4b5e, 0xb4, 0xc8, 0x9c, 0x0, 0x70, 0x92, 0x0, 0xeb);

#define BATTBUS_IOCTL(_index_) \
    CTL_CODE(FILE_DEVICE_BUS_EXTENDER, _index_, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_BATTBUS_PLUGIN_HARDWARE                   BATTBUS_IOCTL(0x0)
#define IOCTL_BATTBUS_UNPLUG_HARDWARE                   BATTBUS_IOCTL(0x1)

//
// Simulated battery ioctl interface
//

// {DAD1F940-CDD0-461f-B23F-C2C663D6E9EB}
DEFINE_GUID(SIMBATT_DEVINTERFACE_GUID,
    0xdad1f940, 0xcdd0, 0x461f, 0xb2, 0x3f, 0xc2, 0xc6, 0x63, 0xd6, 0xe9, 0xeb);

#define SIMBATT_IOCTL(_index_) \
    CTL_CODE(FILE_DEVICE_BATTERY, _index_, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_SIMBATT_SET_STATUS                        SIMBATT_IOCTL(0x800)
#define IOCTL_SIMBATT_SET_DEVICE_NAME                   SIMBATT_IOCTL(0x801)
#define IOCTL_SIMBATT_SET_ESTIMATED_TIME                SIMBATT_IOCTL(0x802)
#define IOCTL_SIMBATT_SET_GRANULARITY_INFORMATION       SIMBATT_IOCTL(0x803)
#define IOCTL_SIMBATT_SET_INFORMATION                   SIMBATT_IOCTL(0x804)
#define IOCTL_SIMBATT_SET_MANUFACTURE_DATE              SIMBATT_IOCTL(0x805)
#define IOCTL_SIMBATT_SET_MANUFACTURE_NAME              SIMBATT_IOCTL(0x806)
#define IOCTL_SIMBATT_SET_SERIAL_NUMBER                 SIMBATT_IOCTL(0x807)
#define IOCTL_SIMBATT_SET_TEMPERATURE                   SIMBATT_IOCTL(0x808)
#define IOCTL_SIMBATT_SET_UNIQUE_ID                     SIMBATT_IOCTL(0x809)
#define IOCTL_SIMBATT_GET_MAXCHARGINGCURRENT            SIMBATT_IOCTL(0x810)
#define SIMBATT_RATE_CALCULATE                          0x7fffffff
#define MAX_SUPPORTED_SIMBATT_CHILDREN                  20

//------------------------------------------------------------------- Data Types

//
//  Data structure used in PlugIn and UnPlug ioctls
//

#define BATTBUS_TYPE_SIMBATT 0;

typedef struct _BATTBUS_PLUGIN_HARDWARE
{
    //
    // Size of this type.
    //

    ULONG Size;

    //
    // Unique serial number of the device to be enumerated.
    // Enumeration will be failed if another device on the
    // bus has the same serial number.
    //

    ULONG SerialNo;

    //
    // UI number.
    //

    ULONG UINumber;

    //
    // Type of device being enumerated
    //
    // Reserved value, set to 0.
    //

    ULONG Type;

} BATTBUS_PLUGIN_HARDWARE, *PBATTBUS_PLUGIN_HARDWARE;

typedef struct _BATTBUS_UNPLUG_HARDWARE
{
    //
    // size of this type
    //

    ULONG Size;

    //
    // Serial number of the device to be unplugged.
    //

    ULONG SerialNo;

    //
    // Must not be referenced used.
    //

    ULONG Reserved[2];

} BATTBUS_UNPLUG_HARDWARE, *PBATTBUS_UNPLUG_HARDWARE;

