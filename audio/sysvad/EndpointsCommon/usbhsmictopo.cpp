/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhsmictopo.cpp

Abstract:

    Implementation of topology miniport for the mic (external: headphone).

--*/
#ifdef SYSVAD_USB_SIDEBAND

#pragma warning (disable : 4127)

#include <sysvad.h>
#include "simple.h"
#include "mintopo.h"
#include "usbhstopo.h"
#include "usbhsmictopo.h"
#include "usbhsmictoptable.h"

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_UsbHsMicTopoFilter
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

    DPF_ENTER(("[PropertyHandler_UsbHsMicTopoFilter]"));

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
                    ARRAYSIZE(UsbHsMicJackDescriptions),
                    UsbHsMicJackDescriptions);
                break;
            
            case KSPROPERTY_JACK_DESCRIPTION2:
                ntStatus = PropertyHandler_UsbHsJackDescription2(
                    PropertyRequest,
                    ARRAYSIZE(UsbHsMicJackDescriptions),
                    UsbHsMicJackDescriptions);
                break;
            
            case KSPROPERTY_JACK_CONTAINERID:
                ntStatus = PropertyHandler_UsbHsJackContainerId(
                    PropertyRequest,
                    ARRAYSIZE(UsbHsMicJackDescriptions),
                    UsbHsMicJackDescriptions);
                break;            
        }
    }
    
    return ntStatus;
} // PropertyHandler_UsbHsMicTopoFilter

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_UsbHsMicVolumeLevel
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

    DPF_ENTER(("[PropertyHandler_UsbHsMicVolumeLevel]"));

    NTSTATUS                ntStatus        = STATUS_INVALID_DEVICE_REQUEST;
    PCMiniportTopology      miniport        = (PCMiniportTopology)PropertyRequest->MajorTarget;
    PSIDEBANDDEVICECOMMON     usbHsDevice    = NULL;
    ULONG                   channel         = (ULONG)-1;
    
    usbHsDevice = miniport->GetSidebandDevice(); // weak ref.
    ASSERT(usbHsDevice != NULL);
    
    if (usbHsDevice->IsVolumeSupported(miniport->m_DeviceType) == FALSE)
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
                ntStatus = usbHsDevice->GetVolume(eUsbHsMicDevice, channel, volume);
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                ntStatus = usbHsDevice->SetVolume(eUsbHsMicDevice, channel, *volume);
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_UsbHsMicMute
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Audio, KSPROPERTY_AUDIO_MUTE )

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_UsbHsMicMute]"));

    NTSTATUS                ntStatus        = STATUS_INVALID_DEVICE_REQUEST;
    PCMiniportTopology      miniport        = (PCMiniportTopology)PropertyRequest->MajorTarget;
    PSIDEBANDDEVICECOMMON   usbHsDevice    = NULL;
    ULONG                   channel         = (ULONG)-1;
    
    usbHsDevice = miniport->GetSidebandDevice(); // weak ref.
    ASSERT(usbHsDevice != NULL);
    
    if (usbHsDevice->IsMuteSupported(miniport->m_DeviceType) == FALSE)
    {
       ntStatus = miniport->PropertyHandlerGeneric(PropertyRequest); 
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = PropertyHandler_UsbHsMute_BasicSupport(PropertyRequest);
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
            LONG* mute = (LONG*)PropertyRequest->Value;

            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *mute = usbHsDevice->GetMute(eUsbHsMicDevice, channel);

                ntStatus = STATUS_SUCCESS;

            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                ntStatus = usbHsDevice->SetMute(eUsbHsMicDevice, channel, *mute);
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_UsbHsMicTopoNode
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

    DPF_ENTER(("[PropertyHandler_UsbHsMicTopoNode]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Audio))
    {
        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
        {
            ntStatus = PropertyHandler_UsbHsMicVolumeLevel(PropertyRequest);
        }
        else if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE)
        {
            ntStatus = PropertyHandler_UsbHsMicMute(PropertyRequest);
        }
    }

    return ntStatus;
} // PropertyHandler_UsbHsMicTopoFilter

//=============================================================================
#pragma code_seg()
NTSTATUS 
PropertyHandler_UsbHsMicTopoNodeEvent
(
    _In_    PPCEVENT_REQUEST    EventRequest
)
{
    ASSERT(EventRequest);

    DPF_ENTER(("[PropertyHandler_UsbHsMicTopoNodeEvent]"));

    // Validate the node.
    if (EventRequest->Node != KSNODE_TOPO_VOLUME &&
        EventRequest->Node != KSNODE_TOPO_MUTE)
        return STATUS_INVALID_PARAMETER;

    return PropertyHandler_UsbHsTopoNodeEvent(EventRequest);
}

//=============================================================================
#pragma code_seg()
NTSTATUS
PropertyHandler_UsbHsMicTopoFilterEvent
(
    _In_    PPCEVENT_REQUEST    EventRequest
)
{
    ASSERT(EventRequest);

    DPF_ENTER(("[PropertyHandler_UsbHsMicTopoFilterEvent]"));

    return PropertyHandler_UsbHsTopoNodeEvent(EventRequest);
}

#pragma code_seg()
#endif  // SYSVAD_USB_SIDEBAND


