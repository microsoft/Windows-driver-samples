/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhsspeakertopo.cpp

Abstract:

    Implementation of topology miniport for the USB Headset speaker (external).

--*/
#ifdef SYSVAD_USB_SIDEBAND

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
#endif  // SYSVAD_USB_SIDEBAND


