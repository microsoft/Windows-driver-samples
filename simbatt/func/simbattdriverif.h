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
