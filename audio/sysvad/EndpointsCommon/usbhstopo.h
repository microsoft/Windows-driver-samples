/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhstopo.h

Abstract:

    Declaration of topology miniport for the mic (external: USB headphone).

--*/

#ifndef _SYSVAD_USBHSTOPO_H_
#define _SYSVAD_USBHSTOPO_H_

NTSTATUS
PropertyHandler_UsbHsVolumeLevel_BasicSupport( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_UsbHsMicVolumeLevel(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_UsbHsMute_BasicSupport( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_UsbHsMicMute(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_UsbHsJackDescription( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest, 
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *    JackDescriptions
    );

NTSTATUS
PropertyHandler_UsbHsJackDescription2( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest,
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *    JackDescriptions
    );

NTSTATUS
PropertyHandler_UsbHsJackContainerId( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest,
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *     JackDescriptions
    );

NTSTATUS 
PropertyHandler_UsbHsTopoNodeEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

#endif // _SYSVAD_USBHSTOPO_H_

