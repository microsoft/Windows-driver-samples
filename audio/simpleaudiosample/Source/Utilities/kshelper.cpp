/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    kshelper.cpp

Abstract:

    Helper functions for simple audio sample
--*/

#include "definitions.h"

//4127: conditional expression is constant
#pragma warning (disable : 4127)

//-----------------------------------------------------------------------------
#pragma code_seg("PAGE")
PWAVEFORMATEX                   
GetWaveFormatEx
(
    _In_  PKSDATAFORMAT           pDataFormat
)
/*++

Routine Description:

  Returns the waveformatex for known formats. 

Arguments:

  pDataFormat - data format.

Return Value:
    
    waveformatex in DataFormat.
    NULL for unknown data formats.

--*/
{
    PAGED_CODE();

    PWAVEFORMATEX           pWfx = NULL;
    
    // If this is a known dataformat extract the waveformat info.
    //
    if
    ( 
        pDataFormat &&
        ( IsEqualGUIDAligned(pDataFormat->MajorFormat, 
                KSDATAFORMAT_TYPE_AUDIO)             &&
          ( IsEqualGUIDAligned(pDataFormat->Specifier, 
                KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
            IsEqualGUIDAligned(pDataFormat->Specifier, 
                KSDATAFORMAT_SPECIFIER_DSOUND) ) )
    )
    {
        pWfx = PWAVEFORMATEX(pDataFormat + 1);

        if (IsEqualGUIDAligned(pDataFormat->Specifier, 
                KSDATAFORMAT_SPECIFIER_DSOUND))
        {
            PKSDSOUND_BUFFERDESC    pwfxds;

            pwfxds = PKSDSOUND_BUFFERDESC(pDataFormat + 1);
            pWfx = &pwfxds->WaveFormatEx;
        }
    }

    return pWfx;
} // GetWaveFormatEx

//-----------------------------------------------------------------------------
#pragma code_seg("PAGE")
NTSTATUS 
ValidatePropertyParams
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest, 
    _In_ ULONG                    cbValueSize,
    _In_ ULONG                    cbInstanceSize /* = 0  */
)
/*++

Routine Description:

  Validates property parameters.

Arguments:

  PropertyRequest - 
  cbValueSize -
  cbInstanceSize -

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    if (PropertyRequest && cbValueSize)
    {
        // If the caller is asking for ValueSize.
        //
        if (0 == PropertyRequest->ValueSize) 
        {
            PropertyRequest->ValueSize = cbValueSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        // If the caller passed an invalid ValueSize.
        //
        else if (PropertyRequest->ValueSize < cbValueSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else if (PropertyRequest->InstanceSize < cbInstanceSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        // If all parameters are OK.
        // 
        else if (PropertyRequest->ValueSize >= cbValueSize)
        {
            if (PropertyRequest->Value)
            {
                ntStatus = STATUS_SUCCESS;
                //
                // Caller should set ValueSize, if the property 
                // call is successful.
                //
            }
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    
    // Clear the ValueSize if unsuccessful.
    //
    if (PropertyRequest &&
        STATUS_SUCCESS != ntStatus &&
        STATUS_BUFFER_OVERFLOW != ntStatus)
    {
        PropertyRequest->ValueSize = 0;
    }

    return ntStatus;
} // ValidatePropertyParams

//-----------------------------------------------------------------------------
#pragma code_seg("PAGE")
NTSTATUS
SimpleAudioSamplePropertyDispatch
(
    _In_ PPCPROPERTY_REQUEST PropertyRequest
)
/*++
    Handles and dispatches a SIMPLEAUDIOSAMPLE_PROPERTY_ITEM.

    Use this as the property handler only if the property item is a
    SIMPLEAUDIOSAMPLE_PROPERTY_ITEM.
--*/
{
    PAGED_CODE();

    SIMPLEAUDIOSAMPLE_PROPERTY_ITEM* item = (SIMPLEAUDIOSAMPLE_PROPERTY_ITEM*)PropertyRequest->PropertyItem;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (item->SupportHandler != nullptr)
        {
            return item->SupportHandler(PropertyRequest);
        }
        else
        {
            return PropertyHandler_BasicSupport(PropertyRequest, PropertyRequest->PropertyItem->Flags, VT_ILLEGAL);
        }
    }

    // Verify instance data size
    if (PropertyRequest->InstanceSize < item->MinProperty)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ULONG cbMinSize = item->MinData;

    // Verify value size
    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = cbMinSize;
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < cbMinSize)
    {
        PropertyRequest->ValueSize = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        if (item->GetHandler != nullptr)
        {
            return item->GetHandler(PropertyRequest);
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }
    }

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        if (item->SetHandler != nullptr)
        {
            return item->SetHandler(PropertyRequest);
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

//-----------------------------------------------------------------------------
#pragma code_seg("PAGE")
NTSTATUS                        
PropertyHandler_BasicSupport
(
    _In_ PPCPROPERTY_REQUEST         PropertyRequest,
    _In_ ULONG                       Flags,
    _In_ DWORD                       PropTypeSetId
)
/*++

Routine Description:

  Default basic support handler. Basic processing depends on the size of data.
  For ULONG it only returns Flags. For KSPROPERTY_DESCRIPTION, the structure   
  is filled.

Arguments:

  PropertyRequest - 

  Flags - Support flags.

  PropTypeSetId - PropTypeSetId

Return Value:
    
    NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Flags & KSPROPERTY_TYPE_BASICSUPPORT);

    NTSTATUS                    ntStatus = STATUS_INVALID_PARAMETER;

    if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
    {
        // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
        //
        PKSPROPERTY_DESCRIPTION PropDesc = 
            PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags       = Flags;
        PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
        if  (VT_ILLEGAL != PropTypeSetId)
        {
            PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
            PropDesc->PropTypeSet.Id    = PropTypeSetId;
        }
        else
        {
            PropDesc->PropTypeSet.Set   = GUID_NULL;
            PropDesc->PropTypeSet.Id    = 0;
        }
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 0;
        PropDesc->Reserved          = 0;

        PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        ntStatus = STATUS_SUCCESS;
    } 
    else if (PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        //
        *(PULONG(PropertyRequest->Value)) = Flags;

        PropertyRequest->ValueSize = sizeof(ULONG);
        ntStatus = STATUS_SUCCESS;                    
    }
    else
    {
        PropertyRequest->ValueSize = 0;
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    return ntStatus;
} // PropertyHandler_BasicSupport

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_BasicSupportVolume
(
    _In_  PPCPROPERTY_REQUEST   PropertyRequest,
    _In_  ULONG                 MaxChannels
)
/*++

Routine Description:

  Handles BasicSupport for Volume nodes.

Arguments:
    
  PropertyRequest - property request structure.

  MaxChannels - # of supported channels.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    ULONG                       cbFullProperty = 
        sizeof(KSPROPERTY_DESCRIPTION) +
        sizeof(KSPROPERTY_MEMBERSHEADER) +
        sizeof(KSPROPERTY_STEPPING_LONG) * MaxChannels;
    
    ASSERT(MaxChannels > 0);

    if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        PKSPROPERTY_DESCRIPTION PropDesc = 
            PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags       = KSPROPERTY_TYPE_ALL;
        PropDesc->DescriptionSize   = cbFullProperty;
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_I4;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer can also hold a range description, return it too
        if(PropertyRequest->ValueSize >= cbFullProperty)
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = 
                PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = MaxChannels;
            Members->Flags          = KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = 
                PKSPROPERTY_STEPPING_LONG(Members + 1);

            for (ULONG i=0; i<MaxChannels; ++i)
            {
                Range[i].Bounds.SignedMaximum = VOLUME_SIGNED_MAXIMUM;   //   0 dB
                Range[i].Bounds.SignedMinimum = VOLUME_SIGNED_MINIMUM;   // -96 dB
                Range[i].SteppingDelta        = VOLUME_STEPPING_DELTA;   //  .5 dB
                Range[i].Reserved             = 0;
            }

            // set the return value size
            PropertyRequest->ValueSize = cbFullProperty;
        } 
        else
        {
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        }
    } 
    else if(PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = PULONG(PropertyRequest->Value);

        PropertyRequest->ValueSize = sizeof(ULONG);
        *AccessFlags = KSPROPERTY_TYPE_ALL;
    }
    else
    {
        PropertyRequest->ValueSize = 0;
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    return ntStatus;
} // PropertyHandlerBasicSupportVolume

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_BasicSupportMute
(
    _In_  PPCPROPERTY_REQUEST   PropertyRequest,
    _In_  ULONG                 MaxChannels
)
/*++

Routine Description:

  Handles BasicSupport for Mute nodes.

Arguments:
    
  PropertyRequest - property request structure.

  MaxChannels - # of supported channels.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    ULONG                       cbFullProperty = 
        sizeof(KSPROPERTY_DESCRIPTION) +
        sizeof(KSPROPERTY_MEMBERSHEADER) +
        sizeof(KSPROPERTY_STEPPING_LONG) * MaxChannels;

    ASSERT(MaxChannels > 0);
    
    if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        PKSPROPERTY_DESCRIPTION PropDesc = 
            PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags       = KSPROPERTY_TYPE_ALL;
        PropDesc->DescriptionSize   = cbFullProperty;
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_BOOL;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer can also hold a range description, return it too
        if(PropertyRequest->ValueSize >= cbFullProperty)
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = 
                PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = MaxChannels;
            Members->Flags          = KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = 
                PKSPROPERTY_STEPPING_LONG(Members + 1);

            for (ULONG i=0; i<MaxChannels; ++i)
            {
                Range[i].Bounds.SignedMaximum = 1;        // true
                Range[i].Bounds.SignedMinimum = 0;        // false
                Range[i].SteppingDelta        = 1;        // false <- -> true
                Range[i].Reserved             = 0;
            }

            // set the return value size
            PropertyRequest->ValueSize = cbFullProperty;
        } 
        else
        {
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        }
    } 
    else if(PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = PULONG(PropertyRequest->Value);

        PropertyRequest->ValueSize = sizeof(ULONG);
        *AccessFlags = KSPROPERTY_TYPE_ALL;
    }
    else
    {
        PropertyRequest->ValueSize = 0;
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    return ntStatus;
} // PropertyHandlerBasicSupportVolume

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_BasicSupportPeakMeter2
(
    _In_  PPCPROPERTY_REQUEST   PropertyRequest,
    _In_  ULONG                 MaxChannels
)
/*++

Routine Description:

  Handles BasicSupport for peak meter nodes.

Arguments:
    
  PropertyRequest - property request structure.

  MaxChannels - # of supported channels.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    ULONG                       cbFullProperty = 
        sizeof(KSPROPERTY_DESCRIPTION) +
        sizeof(KSPROPERTY_MEMBERSHEADER) +
        sizeof(KSPROPERTY_STEPPING_LONG) * MaxChannels;
    
    ASSERT(MaxChannels > 0);

    if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        PKSPROPERTY_DESCRIPTION PropDesc = 
            PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags       = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
        PropDesc->DescriptionSize   = cbFullProperty;
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_I4;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer can also hold a range description, return it too
        if(PropertyRequest->ValueSize >= cbFullProperty)
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = 
                PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = MaxChannels;
            Members->Flags          = KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = 
                PKSPROPERTY_STEPPING_LONG(Members + 1);

            for (ULONG i=0; i<MaxChannels; ++i)
            {
                Range[i].Bounds.SignedMaximum = PEAKMETER_SIGNED_MAXIMUM;
                Range[i].Bounds.SignedMinimum = PEAKMETER_SIGNED_MINIMUM;
                Range[i].SteppingDelta        = PEAKMETER_STEPPING_DELTA;
                Range[i].Reserved             = 0;
            }

            // set the return value size
            PropertyRequest->ValueSize = cbFullProperty;
        } 
        else
        {
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        }
    } 
    else if(PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = PULONG(PropertyRequest->Value);

        PropertyRequest->ValueSize = sizeof(ULONG);
        *AccessFlags = KSPROPERTY_TYPE_ALL;
    }
    else
    {
        PropertyRequest->ValueSize = 0;
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    return ntStatus;
} // PropertyHandlerBasicSupportVolume

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_CpuResources
( 
    _In_  PPCPROPERTY_REQUEST     PropertyRequest 
)
/*++

Routine Description:

  Processes KSPROPERTY_AUDIO_CPURESOURCES

Arguments:
    
  PropertyRequest - property request structure.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[%s]",__FUNCTION__));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        ntStatus = ValidatePropertyParams(PropertyRequest, sizeof(ULONG));
        if (NT_SUCCESS(ntStatus))
        {
            *(PULONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
            PropertyRequest->ValueSize = sizeof(ULONG);
        }
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = 
            PropertyHandler_BasicSupport
            ( 
                PropertyRequest, 
                KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
                VT_UI4
            );
    }

    return ntStatus;
} // PropertyHandlerCpuResources

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_Volume
(
    _In_  PADAPTERCOMMON        AdapterCommon,
    _In_  PPCPROPERTY_REQUEST   PropertyRequest,
    _In_  ULONG                 MaxChannels
)
/*++

Routine Description:

  Property handler for KSPROPERTY_AUDIO_VOLUMELEVEL

Arguments:

  AdapterCommon - interface to the common adapter object.
  
  PropertyRequest - property request structure.

  MaxChannels - # of supported channels.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[%s]",__FUNCTION__));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG    ulChannel;
    PLONG    plVolume;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = PropertyHandler_BasicSupportVolume(
                            PropertyRequest,
                            MaxChannels);
    }
    else
    {
        ntStatus = 
            ValidatePropertyParams
            (
                PropertyRequest, 
                sizeof(LONG),    // volume value is a LONG
                sizeof(ULONG)    // instance is the channel number
            );
        if (NT_SUCCESS(ntStatus))
        {
            ulChannel = * (PULONG (PropertyRequest->Instance));
            plVolume  = PLONG (PropertyRequest->Value);

            if (ulChannel >= MaxChannels &&
                ulChannel != ALL_CHANNELS_ID)
            {
               ntStatus = STATUS_INVALID_PARAMETER;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *plVolume = 
                    AdapterCommon->MixerVolumeRead
                    (
                        PropertyRequest->Node, 
                        ulChannel == ALL_CHANNELS_ID ? 0 : ulChannel
                    );
                PropertyRequest->ValueSize = sizeof(ULONG);                
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                if (ALL_CHANNELS_ID == ulChannel)
                {
                    for (ULONG i=0; i<ulChannel; ++i)
                    {
                        AdapterCommon->MixerVolumeWrite
                        (
                            PropertyRequest->Node, 
                            i, 
                            VOLUME_NORMALIZE_IN_RANGE(*plVolume)
                        );
                    }
                }
                else 
                {
                    AdapterCommon->MixerVolumeWrite
                    (
                        PropertyRequest->Node, 
                        ulChannel, 
                        VOLUME_NORMALIZE_IN_RANGE(*plVolume)
                    );
                }
            }
        }

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_TERSE, ("[%s - ntStatus=0x%08x]",__FUNCTION__,ntStatus));
        }
    }

    return ntStatus;
} // PropertyHandlerVolume

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS                            
PropertyHandler_Mute
(
    _In_  PADAPTERCOMMON        AdapterCommon,
    _In_  PPCPROPERTY_REQUEST   PropertyRequest,
    _In_  ULONG                 MaxChannels
)
/*++

Routine Description:

  Property handler for KSPROPERTY_AUDIO_MUTE

Arguments:

  AdapterCommon - interface to the common adapter object.
  
  PropertyRequest - property request structure.

  MaxChannels - # of supported channels.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[%s]",__FUNCTION__));

    NTSTATUS                    ntStatus;
    ULONG                       ulChannel;
    PBOOL                       pfMute;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = PropertyHandler_BasicSupportMute(
                            PropertyRequest,
                            MaxChannels);
    }
    else
    {
        ntStatus = 
            ValidatePropertyParams
            (   
                PropertyRequest, 
                sizeof(BOOL), 
                sizeof(ULONG)
            );
        if (NT_SUCCESS(ntStatus))
        {
            ulChannel = * (PULONG (PropertyRequest->Instance));
            pfMute    = PBOOL (PropertyRequest->Value);

            if (ulChannel >= MaxChannels &&
                ulChannel != ALL_CHANNELS_ID)
            {
               ntStatus = STATUS_INVALID_PARAMETER;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *pfMute = 
                    AdapterCommon->MixerMuteRead
                    (
                        PropertyRequest->Node,
                        ulChannel == ALL_CHANNELS_ID ? 0 : ulChannel
                    );
                PropertyRequest->ValueSize = sizeof(BOOL);
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                if (ALL_CHANNELS_ID == ulChannel)
                {
                    for (ULONG i=0; i<ulChannel; ++i)
                    {
                        AdapterCommon->MixerMuteWrite
                        (
                            PropertyRequest->Node,
                            i,
                            (*pfMute) ? TRUE : FALSE
                        );
                    }
                }
                else
                {
                    AdapterCommon->MixerMuteWrite
                    (
                        PropertyRequest->Node,
                        ulChannel,
                        (*pfMute) ? TRUE : FALSE
                    );
                }
            }
        }
        
        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_TERSE, ("[%s - ntStatus=0x%08x]",__FUNCTION__,ntStatus));
        }
    }

    return ntStatus;
} // PropertyHandlerMute

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
PropertyHandler_PeakMeter2
(
    _In_  PADAPTERCOMMON        AdapterCommon,
    _In_  PPCPROPERTY_REQUEST   PropertyRequest,
    _In_  ULONG                 MaxChannels
)
/*++

Routine Description:

  Property handler for KSPROPERTY_AUDIO_PEAKMETER2

Arguments:

  AdapterCommon - interface to the common adapter object.
  
  PropertyRequest - property request structure.

  MaxChannels - # of supported channels.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[%s]",__FUNCTION__));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG    ulChannel;
    PLONG    plSample;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = PropertyHandler_BasicSupportPeakMeter2(
                            PropertyRequest,
                            MaxChannels);
    }
    else
    {
        ntStatus = 
            ValidatePropertyParams
            (
                PropertyRequest, 
                sizeof(LONG),    // sample value is a LONG
                sizeof(ULONG)    // instance is the channel number
            );
        if (NT_SUCCESS(ntStatus))
        {
            ulChannel = * (PULONG (PropertyRequest->Instance));
            plSample  = PLONG (PropertyRequest->Value);

            if (ulChannel >= MaxChannels &&
                ulChannel != ALL_CHANNELS_ID)
            {
               ntStatus = STATUS_INVALID_PARAMETER;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *plSample = 
                    PEAKMETER_NORMALIZE_IN_RANGE(
                        AdapterCommon->MixerPeakMeterRead
                        (
                            PropertyRequest->Node, 
                            ulChannel == ALL_CHANNELS_ID ? 0 : ulChannel
                        ));
                
                PropertyRequest->ValueSize = sizeof(ULONG);                
            }
        }

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_TERSE, ("[%s - ntStatus=0x%08x]",__FUNCTION__,ntStatus));
        }
    }

    return ntStatus;
} // PropertyHandlerVolume

