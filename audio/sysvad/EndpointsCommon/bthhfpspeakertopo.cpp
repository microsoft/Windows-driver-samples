/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpspeakertopo.cpp

Abstract:

    Implementation of topology miniport for the Bluetooth Hands-Free Profile speaker (external).

--*/
#ifdef SYSVAD_BTH_BYPASS

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "mintopo.h"
#include "bthhfptopo.h"
#include "bthhfpspeakertopo.h"
#include "bthhfpspeakertoptable.h"

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_BthHfpSpeakerTopoFilter
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

    DPF_ENTER(("[PropertyHandler_BthHfpSpeakerTopoFilter]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    //
    // This line shows how to get a pointer to the miniport topology object.
    //
    PCMiniportTopology  pMiniport = (PCMiniportTopology)PropertyRequest->MajorTarget;
    UNREFERENCED_VAR(pMiniport);

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Jack))
    {
        switch(PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_JACK_DESCRIPTION:
                ntStatus = PropertyHandler_BthHfpJackDescription(
                    PropertyRequest,
                    ARRAYSIZE(BthHfpSpeakerJackDescriptions),
                    BthHfpSpeakerJackDescriptions);
                break;
                
            case KSPROPERTY_JACK_DESCRIPTION2:
                ntStatus = PropertyHandler_BthHfpJackDescription2(
                    PropertyRequest,
                    ARRAYSIZE(BthHfpSpeakerJackDescriptions),
                    BthHfpSpeakerJackDescriptions);
                break;
                
            case KSPROPERTY_JACK_CONTAINERID:
                ntStatus = PropertyHandler_BthHfpJackContainerId(
                    PropertyRequest,
                    ARRAYSIZE(BthHfpSpeakerJackDescriptions),
                    BthHfpSpeakerJackDescriptions);
                break;
        }
    }
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_BtAudio))
    {
        switch(PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_ONESHOT_RECONNECT:
                ntStatus = PropertyHandler_BthHfpOneShotReconnect(PropertyRequest);
                break;
                
            case KSPROPERTY_ONESHOT_DISCONNECT:
                ntStatus = PropertyHandler_BthHfpOneDisconnect(PropertyRequest);
                break;
        }
    }

    return ntStatus;
} // PropertyHandler_BthHfpSpeakerTopoFilter

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_BthHfpSpeakerVolumeLevel
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Audio, KSPROPERTY_AUDIO_VOLUMELEVEL )

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_BthHfpSpeakerVolumeLevel]"));

    NTSTATUS                ntStatus        = STATUS_INVALID_DEVICE_REQUEST;
    PCMiniportTopology      miniport        = (PCMiniportTopology)PropertyRequest->MajorTarget;
    PBTHHFPDEVICECOMMON     bthHfpDevice    = NULL;
    ULONG                   channel         = (ULONG)-1;
    
    bthHfpDevice = miniport->GetBthHfpDevice(); // weak ref.
    ASSERT(bthHfpDevice != NULL);

    if (bthHfpDevice->IsVolumeSupported() == FALSE)
    {
       ntStatus = miniport->PropertyHandlerGeneric(PropertyRequest);     
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = PropertyHandler_BthHfpVolumeLevel_BasicSupport(PropertyRequest);
    }
    else if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        // Instance is the channel #, Bluetooth HFP supports mono streams.
        channel = *(PULONG(PropertyRequest->Instance));

        if (channel == 0)
        {
            ULONG cbNeeded = sizeof(LONG);

            if (PropertyRequest->ValueSize == 0)
            {
                PropertyRequest->ValueSize = cbNeeded;
                ntStatus = STATUS_BUFFER_OVERFLOW;
            }
            else if (PropertyRequest->ValueSize < cbNeeded)
            {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                LONG* volume = (LONG*)PropertyRequest->Value;
                
                if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
                {
                    *volume = bthHfpDevice->GetSpeakerVolume();

                    ntStatus = STATUS_SUCCESS;

                }
                else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
                {
                    ntStatus = bthHfpDevice->SetSpeakerVolume(*volume);
                }
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_BthHfpSpeakerTopoNode
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

    DPF_ENTER(("[PropertyHandler_BthHfpSpeakerTopoNode]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    
    //
    // This line shows how to get a pointer to the miniport topology object.
    //
    PCMiniportTopology  pMiniport = (PCMiniportTopology)PropertyRequest->MajorTarget;
    UNREFERENCED_VAR(pMiniport);

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Audio))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
        {
            ntStatus = PropertyHandler_BthHfpSpeakerVolumeLevel(PropertyRequest);
        }
    }

    return ntStatus;
} // PropertyHandler_BthHfpSpeakerTopoFilter

//=============================================================================
#pragma code_seg()
NTSTATUS 
PropertyHandler_BthHfpSpeakerTopoNodeEvent
(
    _In_    PPCEVENT_REQUEST    EventRequest
)
{
    ASSERT(EventRequest);

    DPF_ENTER(("[PropertyHandler_BthHfpSpeakerTopoNodeEvent]"));

    // Validate the node.
    if (EventRequest->Node != KSNODE_TOPO_VOLUME)
        return STATUS_INVALID_PARAMETER;

    return PropertyHandler_BthHfpTopoNodeEvent(EventRequest);
}

//=============================================================================
#pragma code_seg()
NTSTATUS
PropertyHandler_BthHfpSpeakerTopoFilterEvent
(
    _In_    PPCEVENT_REQUEST    EventRequest
)
{
    ASSERT(EventRequest);

    DPF_ENTER(("[PropertyHandler_BthHfpSpeakerTopoFilterEvent]"));

    return PropertyHandler_BthHfpTopoNodeEvent(EventRequest);
}

#pragma code_seg()
#endif  // SYSVAD_BTH_BYPASS


