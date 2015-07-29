
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfptopo.h

Abstract:

    Declaration of topology miniport for the mic (external: headphone).

--*/

#ifndef _SYSVAD_BTHHFPTOPO_H_
#define _SYSVAD_BTHHFPTOPO_H_

NTSTATUS
PropertyHandler_BthHfpVolumeLevel_BasicSupport( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_BthHfpJackDescription( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest, 
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *    JackDescriptions
    );

NTSTATUS
PropertyHandler_BthHfpJackDescription2( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest,
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *    JackDescriptions
    );

NTSTATUS
PropertyHandler_BthHfpJackContainerId( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest,
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *     JackDescriptions
    );

NTSTATUS
PropertyHandler_BthHfpOneShotReconnect( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS
PropertyHandler_BthHfpOneDisconnect( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS 
PropertyHandler_BthHfpTopoNodeEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

#endif // _SYSVAD_BTHHFPTOPO_H_

