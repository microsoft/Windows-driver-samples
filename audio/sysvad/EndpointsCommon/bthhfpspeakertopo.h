
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpspeakertopo.h

Abstract:

    Declaration of topology miniport for the Bluetooth Hands-Free Profile speaker (external).

--*/

#ifndef _SYSVAD_BTHHFPSPEAKERTOPO_H_
#define _SYSVAD_BTHHFPSPEAKERTOPO_H_

// Function declarations.
NTSTATUS
PropertyHandler_BthHfpSpeakerTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS
PropertyHandler_BthHfpSpeakerTopoNode( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS 
PropertyHandler_BthHfpSpeakerTopoNodeEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

NTSTATUS
PropertyHandler_BthHfpSpeakerTopoFilterEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

#endif // _SYSVAD_BTHHFPSPEAKERTOPO_H_

