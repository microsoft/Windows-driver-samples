/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    a2dphptopo.h

Abstract:

    Declaration of topology miniport for the mic (external: A2DP headphone).

--*/

#ifndef _SYSVAD_A2DPHPTOPO_H_
#define _SYSVAD_A2DPHPTOPO_H_

NTSTATUS
PropertyHandler_A2dpHpVolumeLevel_BasicSupport( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_A2dpHpMicVolumeLevel(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_A2dpHpMute_BasicSupport( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_A2dpHpMicMute(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
    );

NTSTATUS
PropertyHandler_A2dpHpJackDescription( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest, 
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *    JackDescriptions
    );

NTSTATUS
PropertyHandler_A2dpHpJackDescription2( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest,
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *    JackDescriptions
    );

NTSTATUS
PropertyHandler_A2dpHpJackContainerId( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest,
    _In_ ULONG                    cJackDescriptions,
    _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *     JackDescriptions
    );

NTSTATUS 
PropertyHandler_A2dpHpTopoNodeEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

NTSTATUS
PropertyHandler_A2dpHpOneShotReconnect(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
);

NTSTATUS
PropertyHandler_A2dpHpOneShotDisconnect(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
);

#endif // _SYSVAD_A2DPHPTOPO_H_

