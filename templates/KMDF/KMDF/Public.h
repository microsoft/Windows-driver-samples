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
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_KMDF,
    0x8584fca7,0x7a47,0x4056,0x8b,0x7e,0x11,0xa3,0x0a,0x44,0x78,0xe9);
// {8584fca7-7a47-4056-8b7e-11a30a4478e9}
