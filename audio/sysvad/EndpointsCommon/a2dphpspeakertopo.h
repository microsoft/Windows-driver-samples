/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    a2dphpspeakertopo.h

Abstract:

    Declaration of topology miniport for the A2DP Headphone speaker (external).

--*/

#ifndef _SYSVAD_A2DPHPSPEAKERTOPO_H_
#define _SYSVAD_A2DPHPSPEAKERTOPO_H_

// Function declarations.
NTSTATUS
PropertyHandler_A2dpHpSpeakerTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS
PropertyHandler_A2dpHpSpeakerTopoFilterEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

#endif // _SYSVAD_A2DPHPSPEAKERTOPO_H_

