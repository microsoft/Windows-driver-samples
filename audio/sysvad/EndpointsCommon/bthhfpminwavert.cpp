/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpminwavert.cpp

Abstract:

    Implementation of wavert miniport.

--*/
#ifdef SYSVAD_BTH_BYPASS

#pragma warning (disable : 4127)

#include <sysvad.h>
#include <limits.h>
#include "simple.h"
#include "minwavert.h"
#include "minwavertstream.h"
#include "IHVPrivatePropertySet.h"


//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRT::PropertyHandler_BthHfpAudioEffectsDiscoveryEffectsList  
(
    _In_ PPCPROPERTY_REQUEST PropertyRequest
)
/*++

Routine Description:

  Handles ( KSPROPSETID_AudioEffectsDiscovery, KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST )

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[PropertyHandler_BthHfpAudioEffectsDiscoveryEffectsList]"));

    NTSTATUS    ntStatus    = STATUS_INVALID_PARAMETER;
    ULONG       nPinId      = (ULONG)-1;

    // 
    // Validate Pin ID.
    //
    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        nPinId = *(PULONG(PropertyRequest->Instance));

        // This prop is valid only on streaming pins.
        if (IsSystemRenderPin(nPinId) || IsSystemCapturePin(nPinId))
        {
            ntStatus = STATUS_SUCCESS;
        }
        else if (IsBridgePin(nPinId) ||
                 IsLoopbackPin(nPinId) ||
                  IsOffloadPin(nPinId))
        {
             ntStatus = STATUS_NOT_SUPPORTED;
        }
    }

    IF_FAILED_JUMP(ntStatus, Done);

    //
    // Valid actions: get and basicsupport.
    //
    ntStatus = STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = 
            PropertyHandler_BasicSupport
            (
                PropertyRequest,
                KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET,
                VT_ILLEGAL
            );
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        ASSERT(m_BthHfpDevice != NULL);

        //
        // bHfpNrecPresent is set to TRUE when the HF (remote device) handles NR + EC.
        // Note that GetNRECDisableStatus() returns TRUE if the HF notified the AG to disable
        // the NR + EC locally.
        //
        BOOL bHfpNrecPresent = m_BthHfpDevice->IsNRECSupported() && m_BthHfpDevice->GetNRECDisableStatus();

        // If it is HFP render pin (data flow in) or if the NR-EC is not supported, 
        // return size 0 effect list
        if (IsSystemRenderPin(nPinId) || !bHfpNrecPresent)
        {
            PropertyRequest->ValueSize = 0;
            ntStatus = STATUS_SUCCESS;
        }
        else
        {
            // Compute total size, two effects: NS and EC (see below).
            ULONG   cbMinSize = sizeof(GUID) * 2;
            
            if (PropertyRequest->ValueSize == 0)
            {
                PropertyRequest->ValueSize = cbMinSize;
                ntStatus = STATUS_BUFFER_OVERFLOW;
            }
            else if (PropertyRequest->ValueSize < cbMinSize)
            {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PGUID effectList = PGUID(PropertyRequest->Value);

                *effectList = AUDIO_EFFECT_TYPE_ACOUSTIC_ECHO_CANCELLATION;
                *(effectList + 1) = AUDIO_EFFECT_TYPE_NOISE_SUPPRESSION;
                
                PropertyRequest->ValueSize = cbMinSize;
                ntStatus = STATUS_SUCCESS;
            } 
        }
    }

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_BthHfpWaveFilter
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Redirects general property request to miniport object

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    CMiniportWaveRT*    waveRt = reinterpret_cast<CMiniportWaveRT*>(PropertyRequest->MajorTarget);

    if (waveRt == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    waveRt->AddRef();
    
    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Pin))
    {
        switch (PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
                ntStatus = waveRt->PropertyHandlerProposedFormat(PropertyRequest);
                break;
        }
    }
    else if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_AudioEffectsDiscovery))
    {
        switch(PropertyRequest->PropertyItem->Id)
        {
            case KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST:
                ntStatus = waveRt->PropertyHandler_BthHfpAudioEffectsDiscoveryEffectsList(PropertyRequest);
                break;
        }
    }
    
    waveRt->Release();

    return ntStatus;
} // PropertyHandler_BthHfpWaveFilter

#pragma code_seg("PAGE")
#endif  // SYSVAD_BTH_BYPASS

  

