/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhsmictopo.h

Abstract:

    Declaration of topology miniport for the mic (external: headphone).

--*/

#ifndef _SYSVAD_USBHSMICTOPO_H_
#define _SYSVAD_USBHSMICTOPO_H_

// Function declarations.
NTSTATUS
PropertyHandler_UsbHsMicTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS
PropertyHandler_UsbHsMicTopoNode( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS 
PropertyHandler_UsbHsMicTopoNodeEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

NTSTATUS
PropertyHandler_UsbHsMicTopoFilterEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

#endif // _SYSVAD_USBHSMICTOPO_H_

