/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Usb.h

Abstract:

    Contains prototypes for interfacing with a USB connected device.  These
    are copied from the KMDF WDFUSB.H header file (but with the WDF specific
    portions removed)

Environment:

    kernel mode only

--*/

#pragma once

typedef enum _WINUSB_BMREQUEST_DIRECTION {
    BmRequestHostToDevice = BMREQUEST_HOST_TO_DEVICE,
    BmRequestDeviceToHost = BMREQUEST_DEVICE_TO_HOST,
} WINUSB_BMREQUEST_DIRECTION;

typedef enum _WINUSB_BMREQUEST_TYPE {
    BmRequestStandard = BMREQUEST_STANDARD,
    BmRequestClass = BMREQUEST_CLASS,
    BmRequestVendor = BMREQUEST_VENDOR,
} WINUSB_BMREQUEST_TYPE;

typedef enum _WINUSB_BMREQUEST_RECIPIENT {
    BmRequestToDevice = BMREQUEST_TO_DEVICE,
    BmRequestToInterface = BMREQUEST_TO_INTERFACE,
    BmRequestToEndpoint = BMREQUEST_TO_ENDPOINT,
    BmRequestToOther = BMREQUEST_TO_OTHER,
} WINUSB_BMREQUEST_RECIPIENT;

typedef enum _WINUSB_DEVICE_TRAITS {
    WINUSB_DEVICE_TRAIT_SELF_POWERED =        0x00000001,
    WINUSB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE = 0x00000002,
    WINUSB_DEVICE_TRAIT_AT_HIGH_SPEED =       0x00000004,
} WINUSB_DEVICE_TRAITS;

typedef enum _WdfUsbTargetDeviceSelectInterfaceType {
    WdfUsbTargetDeviceSelectInterfaceTypeInterface = 0x10,
    WdfUsbTargetDeviceSelectInterfaceTypeUrb = 0x11,
} WdfUsbTargetDeviceSelectInterfaceType;



typedef union _WINUSB_CONTROL_SETUP_PACKET {
    struct {
        union {
            #pragma warning(disable:4214) // bit field types other than int
            struct {
                //
                // Valid values are BMREQUEST_TO_DEVICE, BMREQUEST_TO_INTERFACE,
                // BMREQUEST_TO_ENDPOINT, BMREQUEST_TO_OTHER
                //
                BYTE Recipient:2;

                BYTE Reserved:3;

                //
                // Valid values are BMREQUEST_STANDARD, BMREQUEST_CLASS,
                // BMREQUEST_VENDOR
                //
                BYTE Type:2;

                //
                // Valid values are BMREQUEST_HOST_TO_DEVICE,
                // BMREQUEST_DEVICE_TO_HOST
                //
                BYTE Dir:1;
            } Request;
            #pragma warning(default:4214) // bit field types other than int
            BYTE Byte;
        } bm;

        BYTE bRequest;

        union {
            struct {
                BYTE LowByte;
                BYTE HiByte;
            } Bytes;
            USHORT Value;
        } wValue;

        union {
            struct {
                BYTE LowByte;
                BYTE HiByte;
            } Bytes;
            USHORT Value;
        } wIndex;

        USHORT wLength;
    } Packet;

    struct {
        BYTE Bytes[8];
    } Generic;

    WINUSB_SETUP_PACKET WinUsb;

} WINUSB_CONTROL_SETUP_PACKET, *PWINUSB_CONTROL_SETUP_PACKET;

VOID
FORCEINLINE
WINUSB_CONTROL_SETUP_PACKET_INIT(
    PWINUSB_CONTROL_SETUP_PACKET Packet,
    WINUSB_BMREQUEST_DIRECTION Direction,
    WINUSB_BMREQUEST_RECIPIENT Recipient,
    BYTE Request,
    USHORT Value,
    USHORT Index
    )
{
    RtlZeroMemory(Packet, sizeof(WINUSB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) Direction;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestStandard;
    Packet->Packet.bm.Request.Recipient = (BYTE) Recipient;

    Packet->Packet.bRequest = Request;
    Packet->Packet.wValue.Value = Value;
    Packet->Packet.wIndex.Value = Index;

    // Packet->Packet.wLength will be set by the formatting function
}

VOID
FORCEINLINE
WINUSB_CONTROL_SETUP_PACKET_INIT_CLASS(
    PWINUSB_CONTROL_SETUP_PACKET Packet,
    WINUSB_BMREQUEST_DIRECTION Direction,
    WINUSB_BMREQUEST_RECIPIENT Recipient,
    BYTE Request,
    USHORT Value,
    USHORT Index
    )
{
    RtlZeroMemory(Packet, sizeof(WINUSB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) Direction;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestClass;
    Packet->Packet.bm.Request.Recipient = (BYTE) Recipient;

    Packet->Packet.bRequest = Request;
    Packet->Packet.wValue.Value = Value;
    Packet->Packet.wIndex.Value = Index;

    // Packet->Packet.wLength will be set by the formatting function
}

VOID
FORCEINLINE
WINUSB_CONTROL_SETUP_PACKET_INIT_VENDOR(
    PWINUSB_CONTROL_SETUP_PACKET Packet,
    WINUSB_BMREQUEST_DIRECTION Direction,
    WINUSB_BMREQUEST_RECIPIENT Recipient,
    BYTE Request,
    USHORT Value,
    USHORT Index
    )
{
    RtlZeroMemory(Packet, sizeof(WINUSB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) Direction;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestVendor;
    Packet->Packet.bm.Request.Recipient = (BYTE) Recipient;

    Packet->Packet.bRequest = Request;
    Packet->Packet.wValue.Value = Value;
    Packet->Packet.wIndex.Value = Index;

    // Packet->Packet.wLength will be set by the formatting function
}

VOID
FORCEINLINE
WINUSB_CONTROL_SETUP_PACKET_INIT_FEATURE(
    PWINUSB_CONTROL_SETUP_PACKET Packet,
    WINUSB_BMREQUEST_RECIPIENT BmRequestRecipient,
    USHORT FeatureSelector,
    USHORT Index,
    BOOLEAN SetFeature
    )
{
    RtlZeroMemory(Packet, sizeof(WINUSB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) BmRequestHostToDevice;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestStandard;
    Packet->Packet.bm.Request.Recipient = (BYTE) BmRequestRecipient;

    if (SetFeature) {
        Packet->Packet.bRequest = USB_REQUEST_SET_FEATURE;
    }
    else {
        Packet->Packet.bRequest = USB_REQUEST_CLEAR_FEATURE;
    }

    Packet->Packet.wValue.Value = FeatureSelector;
    Packet->Packet.wIndex.Value = Index;

    // Packet->Packet.wLength will be set by the formatting function
}

VOID
FORCEINLINE
WINUSB_CONTROL_SETUP_PACKET_INIT_GET_STATUS(
    PWINUSB_CONTROL_SETUP_PACKET Packet,
    WINUSB_BMREQUEST_RECIPIENT BmRequestRecipient,
    USHORT Index
    )
{
    RtlZeroMemory(Packet, sizeof(WINUSB_CONTROL_SETUP_PACKET));

    Packet->Packet.bm.Request.Dir = (BYTE) BmRequestDeviceToHost;
    Packet->Packet.bm.Request.Type = (BYTE) BmRequestStandard;
    Packet->Packet.bm.Request.Recipient = (BYTE) BmRequestRecipient;

    Packet->Packet.bRequest = USB_REQUEST_GET_STATUS;
    Packet->Packet.wIndex.Value = Index;
    Packet->Packet.wValue.Value = 0;

    // Packet->Packet.wLength will be set by the formatting function
}

