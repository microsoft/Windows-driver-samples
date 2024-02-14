/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    driver and application

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_UMDF2,
    0x8e2c25f0,0xedbe,0x428c,0xac,0xa2,0x7a,0x76,0x27,0xb3,0x43,0x41);
// {8e2c25f0-edbe-428c-aca2-7a7627b34341}
