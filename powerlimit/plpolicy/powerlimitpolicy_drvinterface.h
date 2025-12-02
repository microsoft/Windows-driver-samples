/*++

Copyright (c) Microsoft Corporation

Module Name:

    powerlimitpolicy_drvinterface.h

Abstract:

    This module contains the interfaces used to communicate with the simulate
    power limit policy driver stack.

--*/

//--------------------------------------------------------------------- Pragmas

#pragma once

//--------------------------------------------------------------------- Defines

//
// Simulated power limit policy interface
//

// {dbdc0da1-563c-4e20-8408-d4e3c1069ea3}
DEFINE_GUID(GUID_DEVINTERFACE_POWERLIMIT_POLICY,
0xdbdc0da1, 0x563c, 0x4e20, 0x84, 0x08, 0xd4, 0xe3, 0xc1, 0x06, 0x9e, 0xa3);

//
// IOCTLs to control the policy driver
//

#define POWERLIMIT_POLICY_IOCTL(_index_) \
    CTL_CODE(FILE_DEVICE_UNKNOWN, _index_, METHOD_BUFFERED, FILE_WRITE_DATA)

//
// IOCTL_POWERLIMIT_POLICY_REGISTER
// - Input: WCHAR[], contains interface name of the target device
// - Output: ULONG, Id of the created power limit
//

#define IOCTL_POWERLIMIT_POLICY_REGISTER             POWERLIMIT_POLICY_IOCTL(0x800)

//
// IOCTL_POWERLIMIT_POLICY_UNREGISTER
// - Input: ULONG, Id of the power limit to unregister
//

#define IOCTL_POWERLIMIT_POLICY_UNREGISTER           POWERLIMIT_POLICY_IOCTL(0x801)

//
// IOCTL_POWERLIMIT_POLICY_QUERY_ATTRIBUTES
// - Input: POWERLIMIT_POLICY_ATTRIBUTES
// - Output: POWERLIMIT_POLICY_ATTRIBUTES
//

#define IOCTL_POWERLIMIT_POLICY_QUERY_ATTRIBUTES     POWERLIMIT_POLICY_IOCTL(0x802)

//
// IOCTL_POWERLIMIT_POLICY_QUERY_VALUES
// - Input: POWERLIMIT_POLICY_VALUES
// - Output: POWERLIMIT_POLICY_VALUES
//

#define IOCTL_POWERLIMIT_POLICY_QUERY_VALUES         POWERLIMIT_POLICY_IOCTL(0x803)

//
// IOCTL_POWERLIMIT_POLICY_SET_VALUES
// - Input: POWERLIMIT_POLICY_VALUES, values to be updated for target power limit request
//

#define IOCTL_POWERLIMIT_POLICY_SET_VALUES           POWERLIMIT_POLICY_IOCTL(0x804)

typedef struct _POWERLIMIT_POLICY_ATTRIBUTES {
    ULONG RequestId;
    ULONG BufferCount;
    POWER_LIMIT_ATTRIBUTES Buffer[ANYSIZE_ARRAY];
} POWERLIMIT_POLICY_ATTRIBUTES, *PPOWERLIMIT_POLICY_ATTRIBUTES;

typedef struct _POWERLIMIT_POLICY_VALUES {
    ULONG RequestId;
    ULONG BufferCount;
    POWER_LIMIT_VALUE Buffer[ANYSIZE_ARRAY];
} POWERLIMIT_POLICY_VALUES, *PPOWERLIMIT_POLICY_VALUES;
