/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    DUMP.C

Abstract:

    Routines to dump the descriptors information in a human readable form.

Environment:

    user mode only

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "devioctl.h"

#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <basetyps.h>
#include "usbdi.h"
#include "public.h"

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)

HANDLE
OpenDevice(
    _In_ BOOL Synchronous
    );


char*
usbDescriptorTypeString(UCHAR bDescriptorType )
/*++
Routine Description:

    Called to get ascii string of USB descriptor

Arguments:

        PUSB_ENDPOINT_DESCRIPTOR->bDescriptorType or
        PUSB_DEVICE_DESCRIPTOR->bDescriptorType or
        PUSB_INTERFACE_DESCRIPTOR->bDescriptorType or
        PUSB_STRING_DESCRIPTOR->bDescriptorType or
        PUSB_POWER_DESCRIPTOR->bDescriptorType or
        PUSB_CONFIGURATION_DESCRIPTOR->bDescriptorType

Return Value:

    ptr to string

--*/
{

        switch(bDescriptorType) {

        case USB_DEVICE_DESCRIPTOR_TYPE:
                return "USB_DEVICE_DESCRIPTOR_TYPE";

        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                return "USB_CONFIGURATION_DESCRIPTOR_TYPE";


        case USB_STRING_DESCRIPTOR_TYPE:
                return "USB_STRING_DESCRIPTOR_TYPE";


        case USB_INTERFACE_DESCRIPTOR_TYPE:
                return "USB_INTERFACE_DESCRIPTOR_TYPE";


        case USB_ENDPOINT_DESCRIPTOR_TYPE:
                return "USB_ENDPOINT_DESCRIPTOR_TYPE";


#ifdef USB_POWER_DESCRIPTOR_TYPE // this is the older definintion which is actually obsolete
    // workaround for temporary bug in 98ddk, older USB100.h file
        case USB_POWER_DESCRIPTOR_TYPE:
                return "USB_POWER_DESCRIPTOR_TYPE";
#endif

#ifdef USB_RESERVED_DESCRIPTOR_TYPE  // this is the current version of USB100.h as in NT5DDK

        case USB_RESERVED_DESCRIPTOR_TYPE:
                return "USB_RESERVED_DESCRIPTOR_TYPE";

        case USB_CONFIG_POWER_DESCRIPTOR_TYPE:
                return "USB_CONFIG_POWER_DESCRIPTOR_TYPE";

        case USB_INTERFACE_POWER_DESCRIPTOR_TYPE:
                return "USB_INTERFACE_POWER_DESCRIPTOR_TYPE";
#endif // for current nt5ddk version of USB100.h

        default:
                return "??? UNKNOWN!!";
        }
}


char *
usbEndPointTypeString(UCHAR bmAttributes)
/*++
Routine Description:

    Called to get ascii string of endpt descriptor type

Arguments:

        PUSB_ENDPOINT_DESCRIPTOR->bmAttributes

Return Value:

    ptr to string

--*/
{
    UINT typ = bmAttributes & USB_ENDPOINT_TYPE_MASK;


    switch( typ) {
    case USB_ENDPOINT_TYPE_INTERRUPT:
        return "USB_ENDPOINT_TYPE_INTERRUPT";

    case USB_ENDPOINT_TYPE_BULK:
        return "USB_ENDPOINT_TYPE_BULK";

    case USB_ENDPOINT_TYPE_ISOCHRONOUS:
        return "USB_ENDPOINT_TYPE_ISOCHRONOUS";

    case USB_ENDPOINT_TYPE_CONTROL:
            return "USB_ENDPOINT_TYPE_CONTROL";

    default:
            return "??? UNKNOWN!!";
    }
}


char *
usbConfigAttributesString(UCHAR bmAttributes)
/*++
Routine Description:

    Called to get ascii string of USB_CONFIGURATION_DESCRIPTOR attributes

Arguments:

        PUSB_CONFIGURATION_DESCRIPTOR->bmAttributes

Return Value:

    ptr to string

--*/
{
    UINT typ = bmAttributes & USB_CONFIG_POWERED_MASK;


    switch( typ) {

    case USB_CONFIG_BUS_POWERED:
            return "USB_CONFIG_BUS_POWERED";

    case USB_CONFIG_SELF_POWERED:
            return "USB_CONFIG_SELF_POWERED";

    case USB_CONFIG_REMOTE_WAKEUP:
            return "USB_CONFIG_REMOTE_WAKEUP";


    default:
            return "??? UNKNOWN!!";
    }
}


void
print_USB_CONFIGURATION_DESCRIPTOR(PUSB_CONFIGURATION_DESCRIPTOR cd)
/*++
Routine Description:

    Called to do formatted ascii dump to console of a USB config descriptor

Arguments:

    ptr to USB configuration descriptor

Return Value:

    none

--*/
{
    printf("\n===================\nUSB_CONFIGURATION_DESCRIPTOR\n");

    printf(
    "bLength = 0x%x, decimal %d\n", cd->bLength, cd->bLength
    );

    printf(
    "bDescriptorType = 0x%x ( %s )\n", cd->bDescriptorType,
            usbDescriptorTypeString( cd->bDescriptorType )
    );

    printf(
    "wTotalLength = 0x%x, decimal %d\n", cd->wTotalLength, cd->wTotalLength
    );

    printf(
    "bNumInterfaces = 0x%x, decimal %d\n", cd->bNumInterfaces, cd->bNumInterfaces
    );

    printf(
    "bConfigurationValue = 0x%x, decimal %d\n",
            cd->bConfigurationValue, cd->bConfigurationValue
    );

    printf(
    "iConfiguration = 0x%x, decimal %d\n", cd->iConfiguration, cd->iConfiguration
    );

    printf(
    "bmAttributes = 0x%x ( %s )\n", cd->bmAttributes,
            usbConfigAttributesString( cd->bmAttributes )
    );

    printf(
    "MaxPower = 0x%x, decimal %d\n", cd->MaxPower, cd->MaxPower
    );
}


void
print_USB_INTERFACE_DESCRIPTOR(PUSB_INTERFACE_DESCRIPTOR id, UINT ix)
/*++
Routine Description:

    Called to do formatted ascii dump to console of a USB interface descriptor

Arguments:

    ptr to USB interface descriptor

Return Value:

    none

--*/
{
    printf("\n-----------------------------\nUSB_INTERFACE_DESCRIPTOR #%d\n", ix);


    printf(
    "bLength = 0x%x\n", id->bLength
    );


    printf(
    "bDescriptorType = 0x%x ( %s )\n", id->bDescriptorType,
            usbDescriptorTypeString( id->bDescriptorType )
    );


    printf(
    "bInterfaceNumber = 0x%x\n", id->bInterfaceNumber
    );
    printf(
    "bAlternateSetting = 0x%x\n", id->bAlternateSetting
    );
    printf(
    "bNumEndpoints = 0x%x\n", id->bNumEndpoints
    );
    printf(
    "bInterfaceClass = 0x%x\n", id->bInterfaceClass
    );
    printf(
    "bInterfaceSubClass = 0x%x\n", id->bInterfaceSubClass
    );
    printf(
    "bInterfaceProtocol = 0x%x\n", id->bInterfaceProtocol
    );
    printf(
    "bInterface = 0x%x\n", id->iInterface
    );
}


void
print_USB_ENDPOINT_DESCRIPTOR(PUSB_ENDPOINT_DESCRIPTOR ed, int i)
/*++
Routine Description:

    Called to do formatted ascii dump to console of a USB endpoint descriptor

Arguments:

    ptr to USB endpoint descriptor,
        index of this endpt in interface desc

Return Value:

    none

--*/
{
    printf(
        "------------------------------\nUSB_ENDPOINT_DESCRIPTOR for Pipe%02d\n", i
        );

    printf(
        "bLength = 0x%x\n", ed->bLength
        );

    printf(
        "bDescriptorType = 0x%x ( %s )\n", ed->bDescriptorType,
                usbDescriptorTypeString( ed->bDescriptorType )
        );

    if ( USB_ENDPOINT_DIRECTION_IN( ed->bEndpointAddress ) ) {
        printf(
            "bEndpointAddress= 0x%x ( INPUT )\n", ed->bEndpointAddress
            );
    } else {
            printf(
            "bEndpointAddress= 0x%x ( OUTPUT )\n", ed->bEndpointAddress
            );
    }

    printf(
        "bmAttributes= 0x%x ( %s )\n", ed->bmAttributes,
                usbEndPointTypeString ( ed->bmAttributes )
        );

    printf(
        "wMaxPacketSize= 0x%x, decimal %d\n", ed->wMaxPacketSize,
                ed->wMaxPacketSize
        );

    printf(
        "bInterval = 0x%x, decimal %d\n", ed->bInterval, ed->bInterval
        );
}


BOOL
DumpUsbConfig()
/*++
Routine Description:

    Called to do formatted ascii dump to console of USB
    configuration, interface, and endpoint descriptors.

Arguments:

    none

Return Value:

    TRUE or FALSE

--*/
{
    HANDLE hDev;
    UINT success;
    int siz, nBytes;
    char buf[256] = {'\0'};

    hDev = OpenDevice(TRUE);
    if(hDev == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    siz = sizeof(buf);

    success = DeviceIoControl(hDev,
                    IOCTL_OSRUSBFX2_GET_CONFIG_DESCRIPTOR,
                    buf,
                    siz,
                    buf,
                    siz,
                    (PULONG) &nBytes,
                    NULL);

    if(success == FALSE) {
        printf("Ioct - GetConfigDesc failed %d\n", GetLastError());
    } else {

        ULONG i;
        UINT  j, n;
        char *pch;
        PUSB_CONFIGURATION_DESCRIPTOR cd;
        PUSB_INTERFACE_DESCRIPTOR id;
        PUSB_ENDPOINT_DESCRIPTOR ed;

        pch = buf;
        n = 0;

        cd = (PUSB_CONFIGURATION_DESCRIPTOR) pch;

        print_USB_CONFIGURATION_DESCRIPTOR( cd );

        pch += cd->bLength;

        do {
            id = (PUSB_INTERFACE_DESCRIPTOR) pch;

            print_USB_INTERFACE_DESCRIPTOR(id, n++);

            pch += id->bLength;
            for (j=0; j<id->bNumEndpoints; j++) {

                ed = (PUSB_ENDPOINT_DESCRIPTOR) pch;

                print_USB_ENDPOINT_DESCRIPTOR(ed,j);

                pch += ed->bLength;
            }
            i = (ULONG)(pch - buf);

        } while (i<cd->wTotalLength);
    }

    CloseHandle(hDev);

    return success;

}

