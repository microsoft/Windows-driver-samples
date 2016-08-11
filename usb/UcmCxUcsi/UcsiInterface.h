/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    UcsiInterface.h

Abstract:

    Interface to the driver.

Environment:

    Kernel-mode and user-mode.

--*/


#pragma once


// {6C846EEA-9649-46B3-9C37-2556133E5006}
DEFINE_GUID(GUID_DEVINTERFACE_UCSI_TEST,
0x6c846eea, 0x9649, 0x46b3, 0x9c, 0x37, 0x25, 0x56, 0x13, 0x3e, 0x50, 0x6);

//
// A random value from the Microsoft-reserved range.
//
#define FILE_DEVICE_UCSI 4627

#define IOCTL_UCSI_SEND_COMMAND \
    (DWORD) CTL_CODE(FILE_DEVICE_UCSI, \
                     0x401, \
                     METHOD_BUFFERED, \
                     FILE_ANY_ACCESS)

#define IOCTL_UCSI_GET_CCI \
    (DWORD) CTL_CODE(FILE_DEVICE_UCSI, \
                     0x402, \
                     METHOD_BUFFERED, \
                     FILE_ANY_ACCESS)

#define IOCTL_UCSI_GET_MESSAGE \
    (DWORD) CTL_CODE(FILE_DEVICE_UCSI, \
                     0x403, \
                     METHOD_BUFFERED, \
                     FILE_ANY_ACCESS)

//
// An enum defined to make it convenient to pretty-print the IOCTL name
// in WPP.
//

typedef enum _UCSI_IOCTL {

    _IOCTL_UCSI_SEND_COMMAND = IOCTL_UCSI_SEND_COMMAND,
    _IOCTL_UCSI_GET_CCI = IOCTL_UCSI_GET_CCI,
    _IOCTL_UCSI_GET_MESSAGE = IOCTL_UCSI_GET_MESSAGE

} UCSI_IOCTL;

//
// WPP tracing for enums
//

//
// begin_wpp config
//
// CUSTOM_TYPE(UcsiIoctl, ItemEnum(_UCSI_IOCTL));
//
// end_wpp
//
