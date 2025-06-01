/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_KmdfUsb,
    0xd4f6a10a,0xeed5,0x4fdd,0xa3,0x76,0x6c,0x6b,0xf6,0x6d,0x34,0x94);
// {d4f6a10a-eed5-4fdd-a376-6c6bf66d3494}
