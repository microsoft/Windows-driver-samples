
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpmictopo.h

Abstract:

    Declaration of topology miniport for the mic (external: headphone).

--*/

#ifndef _SYSVAD_BTHHFPMICTOPO_H_
#define _SYSVAD_BTHHFPMICTOPO_H_

// Function declarations.
NTSTATUS
PropertyHandler_BthHfpMicTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS
PropertyHandler_BthHfpMicTopoNode( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

NTSTATUS 
PropertyHandler_BthHfpMicTopoNodeEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

NTSTATUS
PropertyHandler_BthHfpMicTopoFilterEvent(
    _In_    PPCEVENT_REQUEST    EventRequest
    );

#endif // _SYSVAD_BTHHFPMICTOPO_H_

