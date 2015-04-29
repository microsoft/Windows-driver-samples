/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    WUDFOsrUsbPublic.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications for the UMDF OSR device sample.

    Note that this driver does NOT use the same device interface GUID
    as the KMDF OSR USB sample.

Environment:

    user and kernel

--*/

#pragma once

//
// Define an Interface Guid so that app can find the device and talk to it.
//

// {573E8C73-0CB4-4471-A1BF-FAB26C31D384}
DEFINE_GUID(GUID_DEVINTERFACE_OSRUSBFX2, 
            0x573e8c73, 0xcb4, 0x4471, 0xa1, 0xbf, 0xfa, 0xb2, 0x6c, 0x31, 0xd3, 0x84);

