/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    NFCSimulatorDDI.h

Abstract:

    This header contains definitions for the NFC simulator driver

Environment:

    User Mode

--*/
#pragma once

// {A965F47F-23E8-4897-90D1-A6C8A1BB4C59}
const GUID GUID_DEVINTERFACE_NFCSIM =
    {0xa965f47f, 0x23e8, 0x4897, {0x90, 0xd1, 0xa6, 0xc8, 0xa1, 0xbb, 0x4c, 0x59}};

// {4D535369-6DD8-4448-4E46-4345452D4944}
const GUID GUID_DH_SECURE_ELEMENT =
    {0x4D535369, 0x6DD8, 0x4448, {0x4E, 0x46, 0x43, 0x45, 0x45, 0x2D, 0x49, 0x44}};

#define IOCTL_NFCSIM_BEGIN_PROXIMITY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NFCSIM_TRIGGER_SEEVENT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1001, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct BEGIN_PROXIMITY_ARGS
{
    WCHAR szName[MAX_PATH];    // Name or IP address
};
