/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhsspeakertopo.cpp

Abstract:

    Implementation of topology miniport for the USB Headset speaker (external).

--*/
//#ifdef SYSVAD_USB_SIDEBAND

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "mintopo.h"
#include "usbhstopo.h"
#include "usbhsspeakertopo.h"
#include "usbhsspeakertoptable.h"

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_UsbHsSpeakerTopoFilter
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

    DPF_ENTER(("[PropertyHandler_UsbHsSpeakerTopoFilter]"));

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
                ntStatus = PropertyHandler_UsbHsJackDescription(
                    PropertyRequest,
                    ARRAYSIZE(UsbHsSpeakerJackDescriptions),
                    UsbHsSpeakerJackDescriptions);
                break;
                
            case KSPROPERTY_JACK_DESCRIPTION2:
                ntStatus = PropertyHandler_UsbHsJackDescription2(
                    PropertyRequest,
                    ARRAYSIZE(UsbHsSpeakerJackDescriptions),
                    UsbHsSpeakerJackDescriptions);
                break;
                
            case KSPROPERTY_JACK_CONTAINERID:
                ntStatus = PropertyHandler_UsbHsJackContainerId(
                    PropertyRequest,
                    ARRAYSIZE(UsbHsSpeakerJackDescriptions),
                    UsbHsSpeakerJackDescriptions);
                break;
        }
    }

    return ntStatus;
} // PropertyHandler_UsbHsSpeakerTopoFilter

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_UsbHsSpeakerVolumeLevel
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

    DPF_ENTER(("[PropertyHandler_UsbHsSpeakerVolumeLevel]"));

    NTSTATUS                ntStatus        = STATUS_INVALID_DEVICE_REQUEST;
    PCMiniportTopology      miniport        = (PCMiniportTopology)PropertyRequest->MajorTarget;
    PSIDEBANDDEVICECOMMON     usbHsDevice    = NULL;
    ULONG                   channel         = (ULONG)-1;
    
    usbHsDevice = miniport->GetUsbHsDevice(); // weak ref.
    ASSERT(usbHsDevice != NULL);

    if (usbHsDevice->IsVolumeSupported() == FALSE)
    {
       ntStatus = miniport->PropertyHandlerGeneric(PropertyRequest);     
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = PropertyHandler_UsbHsVolumeLevel_BasicSupport(PropertyRequest);
    }
    else if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        // Instance is the channel #.
        channel = *(PULONG(PropertyRequest->Instance));

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
                //\TODO: Implement Channel specific volume
                *volume = usbHsDevice->GetSpeakerVolume();

                ntStatus = STATUS_SUCCESS;

            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                //\TODO: Implement Channel specific volume
                ntStatus = usbHsDevice->SetSpeakerVolume(*volume);
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_UsbHsSpeakerTopoNode
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

    DPF_ENTER(("[PropertyHandler_UsbHsSpeakerTopoNode]"));

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
            ntStatus = PropertyHandler_UsbHsSpeakerVolumeLevel(PropertyRequest);
        }
    }

    return ntStatus;
} // PropertyHandler_UsbHsSpeakerTopoFilter

//=============================================================================
#pragma code_seg()
NTSTATUS 
PropertyHandler_UsbHsSpeakerTopoNodeEvent
(
    _In_    PPCEVENT_REQUEST    EventRequest
)
{
    ASSERT(EventRequest);

    DPF_ENTER(("[PropertyHandler_UsbHsSpeakerTopoNodeEvent]"));

    // Validate the node.
    if (EventRequest->Node != KSNODE_TOPO_VOLUME)
        return STATUS_INVALID_PARAMETER;

    return PropertyHandler_UsbHsTopoNodeEvent(EventRequest);
}

//=============================================================================
#pragma code_seg()
NTSTATUS
PropertyHandler_UsbHsSpeakerTopoFilterEvent
(
    _In_    PPCEVENT_REQUEST    EventRequest
)
{
    ASSERT(EventRequest);

    DPF_ENTER(("[PropertyHandler_UsbHsSpeakerTopoFilterEvent]"));

    return PropertyHandler_UsbHsTopoNodeEvent(EventRequest);
}

#pragma code_seg()
//#endif  // SYSVAD_USB_SIDEBAND


