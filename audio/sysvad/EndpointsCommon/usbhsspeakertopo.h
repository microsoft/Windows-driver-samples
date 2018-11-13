/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhsspeakertopo.h

Abstract:

    Declaration of topology miniport for the USB Headset speaker (external).

--*/

#ifndef _SYSVAD_USBHSSPEAKERTOPO_H_
#define _SYSVAD_USBHSSPEAKERTOPO_H_

// Function declarations.
NTSTATUS
PropertyHandler_UsbHsSpeakerTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS
PropertyHandler_UsbHsSpeakerTopoFilterEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

#endif // _SYSVAD_USBHSSPEAKERTOPO_H_

