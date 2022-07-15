/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    a2dphpspeakertopo.cpp

Abstract:

    Implementation of topology miniport for the A2DP Headphone speaker (external).

--*/
#ifdef SYSVAD_A2DP_SIDEBAND

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "mintopo.h"
#include "a2dphptopo.h"
#include "a2dphpspeakertopo.h"
#include "a2dphpspeakertoptable.h"

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_A2dpHpSpeakerTopoFilter
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Redirects property request to miniport object

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_A2dpHpSpeakerTopoFilter]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Jack))
    {
        switch(PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_JACK_DESCRIPTION:
                ntStatus = PropertyHandler_A2dpHpJackDescription(
                    PropertyRequest,
                    ARRAYSIZE(A2dpHpSpeakerJackDescriptions),
                    A2dpHpSpeakerJackDescriptions);
                break;
                
            case KSPROPERTY_JACK_DESCRIPTION2:
                ntStatus = PropertyHandler_A2dpHpJackDescription2(
                    PropertyRequest,
                    ARRAYSIZE(A2dpHpSpeakerJackDescriptions),
                    A2dpHpSpeakerJackDescriptions);
                break;
                
            case KSPROPERTY_JACK_CONTAINERID:
                ntStatus = PropertyHandler_A2dpHpJackContainerId(
                    PropertyRequest,
                    ARRAYSIZE(A2dpHpSpeakerJackDescriptions),
                    A2dpHpSpeakerJackDescriptions);
                break;
        }
    }
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_BtAudio))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_ONESHOT_RECONNECT:
                ntStatus = PropertyHandler_A2dpHpOneShotReconnect(PropertyRequest);
                break;

            case KSPROPERTY_ONESHOT_DISCONNECT:
                ntStatus = PropertyHandler_A2dpHpOneShotDisconnect(PropertyRequest);
                break;
        }
    }

    return ntStatus;
} // PropertyHandler_A2dpHpSpeakerTopoFilter

//=============================================================================
#pragma code_seg()
NTSTATUS
PropertyHandler_A2dpHpSpeakerTopoFilterEvent
(
    _In_    PPCEVENT_REQUEST    EventRequest
)
{
    ASSERT(EventRequest);

    DPF_ENTER(("[PropertyHandler_A2dpHpSpeakerTopoFilterEvent]"));

    return PropertyHandler_A2dpHpTopoNodeEvent(EventRequest);
}

#pragma code_seg()
#endif  // SYSVAD_A2DP_SIDEBAND


