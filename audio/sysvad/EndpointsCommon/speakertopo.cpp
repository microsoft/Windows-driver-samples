/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakertopo.cpp

Abstract:

    Implementation of topology miniport for the speaker (internal).

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "mintopo.h"
#include "speakertopo.h"
#include "speakertoptable.h"


#pragma code_seg("PAGE")
//=============================================================================
NTSTATUS
PropertyHandler_SpeakerTopoFilter
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

    DPF_ENTER(("[PropertyHandler_SpeakerTopoFilter]"));

    // PropertyRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PCMiniportTopology  pMiniport = (PCMiniportTopology)PropertyRequest->MajorTarget;

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Jack))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_JACK_DESCRIPTION)
        {
            ntStatus = pMiniport->PropertyHandlerJackDescription(
                PropertyRequest,
                ARRAYSIZE(SpeakerJackDescriptions),
                SpeakerJackDescriptions
                );
        }
        else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_JACK_DESCRIPTION2)
        {
            ntStatus = pMiniport->PropertyHandlerJackDescription2(
                PropertyRequest,
                ARRAYSIZE(SpeakerJackDescriptions),
                SpeakerJackDescriptions,
                0 // jack capabilities
                );
        }
        else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_JACK_DESCRIPTION3)
        {
            ntStatus = pMiniport->PropertyHandlerJackDescription3(
                PropertyRequest,
                ARRAYSIZE(SpeakerJackDescriptions),
                SpeakerJackDescriptions,
                42 // jack config id
                );
        }
    }
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_AudioResourceManagement))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIORESOURCEMANAGEMENT_RESOURCEGROUP)
        {
            ntStatus = pMiniport->PropertyHandlerAudioResourceGroup(PropertyRequest);
        }
    }
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_AudioPosture))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIOPOSTURE_ORIENTATION)
        {
            ntStatus = pMiniport->PropertyHandlerAudioPostureOrientation(
                PropertyRequest,
                ARRAYSIZE(SpeakerAudioPostureInfoPointers),
                SpeakerAudioPostureInfoPointers
            );
        }
    }

    return ntStatus;
} // PropertyHandler_SpeakerTopoFilter

#pragma code_seg()

