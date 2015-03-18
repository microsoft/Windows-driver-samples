/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#ifndef __PUBLIC_H
#define __PUBLIC_H

#ifdef DEFINE_GUID

//
// Vendor: Define a device interface GUID for Bluetooth Radion On/off.
// Need to use uuidgen to create your own GUID instead of reusing this one.
//
DEFINE_GUID(GUID_DEVINTERFACE_BLUETOOTH_RADIO_ONOFF_VENDOR_SPECIFIC,
        0x98899865, 0x63de, 0x427b, 0x84, 0x77, 0x6c, 0xb7, 0x4d, 0x9f, 0xf2, 0xa7);
//{98899865-63de-427b-8477-6cb74d9ff2a7}

#endif // #ifdef DEFINE_GUID 


//
// IOCTL definitions to support Radio on/off
//
#define FILE_DEVICE_BUSENUM       FILE_DEVICE_BUS_EXTENDER
#define BUSENUM_IOCTL(id, access) CTL_CODE(FILE_DEVICE_BUSENUM, \
                                           (id),                \
                                           METHOD_BUFFERED,     \
                                           access)

#define IOCTL_BUSENUM_SET_RADIO_ONOFF_VENDOR_SPECFIC        BUSENUM_IOCTL(0x1, FILE_WRITE_DATA)

#endif
