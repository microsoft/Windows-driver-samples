/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Rawpdo.h

Abstract:

Environment:

    Kernel mode

--*/

#ifndef _RAWPDO_H
#define _RAWPDO_H

#ifndef RTL_BITS_OF
// This macro is not defined in Win2k ntdef.h
#define RTL_BITS_OF(sizeOfArg) (sizeof(sizeOfArg) * 8)
#endif

//
// Used to identify kbfilter bus. This guid is used as the enumeration string
// for the device id.
DEFINE_GUID(GUID_BUS_OSRUSBFX2_RAWPDO,
0x556cf9d3, 0xe853, 0x4dfb, 0xa2, 0x16, 0x5d, 0x75, 0x6d, 0xf2, 0xc1, 0x7a);
// {556CF9D3-E853-4dfb-A216-5D756DF2C17A}


DEFINE_GUID(GUID_DEVCLASS_OSRUSBFX2,
0x6fde7521, 0x1b65, 0x48ae, 0xb6, 0x28, 0x80, 0xbe, 0x62, 0x1, 0x60, 0x26);
// {6FDE7521-1B65-48ae-B628-80BE62016026}

// \0 in the end is for double termination - required for MULTI_SZ string
#define  OSRUSBFX2_SWITCH_DEVICE_ID L"{6FDE7521-1B65-48ae-B628-80BE62016026}\\OsrUsbFxRawPdo\0"

typedef struct _PDO_IDENTIFICATION_DESCRIPTION
{
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header; // should contain this header

    ULONG SwitchNumber;

} PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;

VOID
OsrFxInitChildList(
    IN PWDFDEVICE_INIT  DeviceInit
    );

EVT_WDF_CHILD_LIST_CREATE_DEVICE OsrEvtDeviceListCreatePdo;

#endif
