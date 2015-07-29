/*++

Module Name:

    MiniportAudioEngineNode.cpp

Abstract:

    Implementation of IMiniportAudioEngineNode interface for wavert miniport.

--*/
#pragma warning (disable : 4127)

#include <sysvad.h>
#include <limits.h>
#include "simple.h"
#include "minwavert.h"
#include "minwavertstream.h"
#include "IHVPrivatePropertySet.h"
#include "UnittestData.h"

#define MINWAVERT_POOLTAG 'RWNM'
//=============================================================================
// IMiniportAudioEngineNode
//=============================================================================

#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetAudioEngineDescriptor 

Description:

    Portcls calls this method to get the pin id for each pin (host process, 
    offload, and loopback) on a specific audio engine node.

Parameters:
    
    _In_ _ulNodeId: node id for the target audio engine node
    _Out_ pAudioEngineDescriptor: a pointer to a KSAUDIOENGINE_DESCRIPTOR to receive the pin id informationived as input to its DriverEntry routine. 

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks:

    The property value is of type KSAUDIOENGINE_DESCRIPTOR and indicates the static properties of the audio engine node.
    The KSAUDIOENGINE_DESCRIPTOR structure is defined as follows:

    typedef struct _tagKSAUDIOENGINE_DESCRIPTOR
    {
        UINT    nHostPinId;
        UINT    nOffloadPinId;
        UINT    nLoopbackPinId; 
    } KSAUDIOENGINE_DESCRIPTOR, *PKSAUDIOENGINE_DESCRIPTOR;

    The fields are defined as:
    nHostPinId – The ID of the pin factory connected to the audio engine node that is intended for host processed audio data.  This is the pin factory on which a software audio engine will run.
    nOffloadPinId – The ID of the pin factory connected to the audio engine node that is intended for offloaded streams.
    nLoopbackPinId – The ID of the pin factory connected to the audio engine that is intended for supplying a post-mix loopback or reference stream.

    All pin ids need to be unique for each audio engine node.

    A wave miniport might have more than one audio engine node and each engine node has their own set of engine pin ids. 
    A driver needs to make sure that the correct set of audio engine descriptor information is returned.
-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetAudioEngineDescriptor(_In_ ULONG _ulNodeId, _Out_ KSAUDIOENGINE_DESCRIPTOR *_pAudioEngineDescriptor)
{
    PAGED_CODE ();
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    ASSERT (_pAudioEngineDescriptor);
    DPF_ENTER(("[CMiniportWaveRT::GetAudioEngineDescriptor]"));
    // In this sample driver, only one single engine node is exposed in its wave filter
    if (_ulNodeId == KSNODE_WAVE_AUDIO_ENGINE)
    {
        _pAudioEngineDescriptor->nHostPinId = GetSystemPinId();
        _pAudioEngineDescriptor->nOffloadPinId = GetOffloadPinId();
        _pAudioEngineDescriptor->nLoopbackPinId = GetLoopbackPinId();
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    return ntStatus;
}
#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetGfxState 

Description:

    Portcls calls this method to get the audio engine GFX's state

Parameters
    
    _In_ _ulNodeId: node id for the target audio engine node
    _Out_ pbEnable: a pointer to a BOOL value for receieving the returned GFX state

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks
    The global operations (on the device stream) inside HW Audio Engine (such as src, dsp, and other special effects) are hidden from the software audio stack.
So, the driver should return TRUE if any one of the effects is on and returns FALSE when all the opertations are off.
-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetGfxState(_In_ ULONG _ulNodeId, _Out_ BOOL *_pbEnable)
{
    PAGED_CODE ();

    DPF_ENTER(("[CMiniportWaveRT::GetGfxState]"));

    UNREFERENCED_PARAMETER(_ulNodeId);

    *_pbEnable = m_bGfxEnabled;
    return STATUS_SUCCESS;
}
#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::SetGfxState 
 
Decscription:

    Portcls calls this method to set the audio engine GFX's state

Parameters:
    
    _In_ _ulNodeId: node id for the target audio engine node
    _In_ bEnable: a BOOL value. TRU: to enable GFX, FALSE: to disable GFX

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks
    The global operations (on the device stream) inside HW Audio Engine (such as src, dsp, and other special effects) are hidden from the software audio stack.
So, when handling disabling GFX, the driver should ALL the effects. When enabling GFX, it's up the driver+HW Audio Engine to decide what opertations to turn on
when they see appropriate.
-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::SetGfxState(_In_ ULONG _ulNodeId, _In_ BOOL _bEnable)
{
    PAGED_CODE ();

    DPF_ENTER(("[CMiniportWaveRT::SetGfxState]"));

    UNREFERENCED_PARAMETER(_ulNodeId);
    
    // see above comments for appropriate enabling/disabling opertations on the HW Audio Engine
    m_bGfxEnabled = _bEnable;
    return STATUS_SUCCESS;
}
#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetEngineFormatSize 
 
Decscription:

    When handling GetMixFormat, DeviceFormat, or SupportDeviceFormatsList, Portcls calls 
    this method to know the correct data size to allocate for the receiving the corresponding
    format information.

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _In_ eEngineFormatType: format target to indicate which format size is being asked
		_Out_ pulFormatSize: a pointer to a ULONG variable for receiving returned szize information

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
NTSTATUS CMiniportWaveRT::GetEngineFormatSize
(
    _In_    ULONG               _ulNodeId,
    _In_    eEngineFormatType   _formatType,
    _Out_   ULONG               *_pulFormatSize
)
{
    PAGED_CODE ();
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DPF_ENTER(("[CMiniportWaveRT::GetEngineFormatSize]"));
    ASSERT (_pulFormatSize);

    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);

    switch (_formatType)
    {
        case eMixFormat:
            DPF_ENTER(("[eMixFormat]"));
            *_pulFormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE);
            break;
        case eDeviceFormat:
            DPF_ENTER(("[eDeviceFormat]"));
            *_pulFormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE);
            break;
        case eSupportedDeviceFormats:
            DPF_ENTER(("[eSupportedDeviceFormats]"));
            *_pulFormatSize = sizeof(KSMULTIPLE_ITEM) + sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE) * GetAudioEngineSupportedDeviceFormats(NULL);
            break;
        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }
Exit:
    return ntStatus;
}
#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetMixFormat 
 
Decscription:

    GetMixFormat returns the current mix format used by the HW Audio Engine

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _Out_ pFormat: a buffer pointer for receiving the mix format information being asked
		_In_ ulBufferSize: a pointer to a ULONG variable that has the size of the buffer pointed by pFormat

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks
  This is a read only operation; the HW Audio Engine mix format is determined by the HW Audio Engine alone.

-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetMixFormat(_In_  ULONG	_ulNodeId, _Out_ KSDATAFORMAT_WAVEFORMATEX *_pFormat, _In_ ULONG _ulBufferSize)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE ();

    ASSERT (_pFormat);

    DPF_ENTER(("[CMiniportWaveRT::GetMixFormat]"));

    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);
    IF_TRUE_ACTION_JUMP(_ulBufferSize < sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE), ntStatus = STATUS_BUFFER_TOO_SMALL, Exit);
#pragma warning(push)
        // IMiniportAudioEngineNode::GetMixFormat's annotation on _pFormat requires it to be KSDATAFORMAT_WAVEFORMATEX.  However,
        // this implementation here will always be called by our own code with _pFormat to be KSDATAFORMAT_WAVEFORMATEXTENSIBLE,
        // so there should be no buffer overrun; also the IF_TRUE_ACTION_JUMP above also help to avoid buffer overrun. 
#pragma warning(disable:6386)
    RtlCopyMemory((PVOID)_pFormat, (PVOID)m_pMixFormat, sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE));
#pragma warning (pop)
    ntStatus = STATUS_SUCCESS;

Exit:    
    return ntStatus;
}
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetDeviceFormat 
 
Decscription:

    GetDeviceFormat returns the current device format used by the HW Audio Engine

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _Out_ pFormat: a buffer pointer for receiving the device format information being asked
		_In_ ulBufferSize: a pointer to a ULONG variable that has the size of the buffer pointed by pFormat

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks
  Setting the device format of a HW Audio Engine could potential impact the mix format inside the HD Audio Engine.
The driver might need to add appropriate src/format converter according or change mix format.

-------------------------------------------------------------------------------------------------------------------------*/
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetDeviceFormat(_In_ ULONG _ulNodeId, _Out_ KSDATAFORMAT_WAVEFORMATEX *_pFormat, _In_ ULONG _ulBufferSize)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PAGED_CODE ();

    ASSERT (_pFormat);

    DPF_ENTER(("[CMiniportWaveRT::GetDeviceFormat]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);
    IF_TRUE_ACTION_JUMP(_ulBufferSize < sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE), ntStatus = STATUS_BUFFER_TOO_SMALL, Exit);
#pragma warning(push)
        // IMiniportAudioEngineNode::GetDeviceFormat's annotation on _pFormat requires it to be KSDATAFORMAT_WAVEFORMATEX.  However,
        // this implementation here will always be called by our own code with _pFormat to be KSDATAFORMAT_WAVEFORMATEXTENSIBLE,
        // so there should be no buffer overrun; also the IF_TRUE_ACTION_JUMP above also help to avoid buffer overrun. 
#pragma warning(disable:6386)
    RtlCopyMemory((PVOID)_pFormat, (PVOID)m_pDeviceFormat, sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE));
#pragma warning (pop)
    ntStatus = STATUS_SUCCESS;

Exit:    
    return ntStatus;
}
#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::SetDeviceFormat 
 
Decscription:

    GetDeviceFormat set the current device format to be used by the HW Audio Engine

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _Out_ pFormat: a buffer pointer with the device format to be set to the hw Audio Engine
		_In_ ulBufferSize: a pointer to a ULONG variable that has the size of the buffer pointed by pFormat

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks
  Setting the device format of a HW Audio Engine could potential impact the mix format inside the HD Audio Engine.
The driver might need to add appropriate src/format converter according or change mix format.

-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::SetDeviceFormat(_In_  ULONG _ulNodeId, _In_ KSDATAFORMAT_WAVEFORMATEX *_pFormat, _In_ ULONG _ulBufferSize)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PAGED_CODE ();

    ASSERT (_pFormat);

    DPF_ENTER(("[CMiniportWaveRT::SetDeviceFormat]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);
    IF_TRUE_ACTION_JUMP(_ulBufferSize < sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE), ntStatus = STATUS_BUFFER_TOO_SMALL, Exit);

    RtlCopyMemory((PVOID)m_pDeviceFormat, (PVOID)_pFormat, sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE));
    ntStatus = STATUS_SUCCESS;

Exit:    
    return ntStatus;
}
#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetSupportedDeviceFormats 
 
Decscription:

    GetSupportedDeviceFormats get the complete format list supported by the hw Audio Engine

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _Out_ pFormat: a buffer pointer for receiving the supported device formats
		_In_ ulBufferSize: a pointer to a ULONG variable that has the size of the buffer pointed by pFormat

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetSupportedDeviceFormats(_In_  ULONG _ulNodeId, _Out_ KSMULTIPLE_ITEM* _pFormat, _In_ ULONG _ulBufferSize)
{
    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE pDeviceFormats;
    ULONG cDeviceFormats;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PAGED_CODE ();

    ASSERT(_pFormat);

    DPF_ENTER(("[CMiniportWaveRT::GetSupportedDeviceFormats]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);

    cDeviceFormats = GetAudioEngineSupportedDeviceFormats(&pDeviceFormats);

    KSMULTIPLE_ITEM *pKsMulti = static_cast<KSMULTIPLE_ITEM*>(_pFormat);
    pKsMulti->Size = sizeof(KSMULTIPLE_ITEM) + sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE) * cDeviceFormats;
    pKsMulti->Count = cDeviceFormats;

    IF_TRUE_ACTION_JUMP(_ulBufferSize < pKsMulti->Size, ntStatus = STATUS_BUFFER_TOO_SMALL, Exit);

    RtlCopyMemory((PVOID)(pKsMulti + 1), pDeviceFormats,
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE) * cDeviceFormats);

    ntStatus = STATUS_SUCCESS;

Exit:    
    return ntStatus;
}
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetDeviceChannelCount 
 
Decscription:

    When handling volume, mute, and meter related KS properties, Portcls calls 
    this method, inside its property handlers, to know the number of channels for the 
    corresponding KS property.

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _In_ _targetType:  the query target (volume. mute, or peak meter)
		_Out_ _pulChannelCount: a pointer to a UINT32 variable for receiving returned channel count information

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetDeviceChannelCount
(
        _In_  ULONG					_ulNodeId,
        _In_  eChannelTargetType	_targetType,
		_Out_  UINT32				*_pulChannelCount
)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE ();
    ASSERT(_pulChannelCount);

    DPF_ENTER(("[CMiniportWaveRT::GetChannelCount]"));

    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);

    switch (_targetType)
    {
        case eVolumeAttribute:
             ntStatus = GetVolumeChannelCount(_pulChannelCount);
             break;
        case eMuteAttribute:
             ntStatus = GetMuteChannelCount(_pulChannelCount);
             break;
        case ePeakMeterAttribute:
             ntStatus = GetPeakMeterChannelCount(_pulChannelCount);
             break;
        default:
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
Exit:
    return ntStatus;
}
#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetDeviceAttributeSteppings 
 
Decscription:

    When handling volume, mute, and meter related KS properties, Portcls calls 
    this method, inside its property handlers, to know the property stepping information for the 
    corresponding KS property.

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _In_ _targetType:  the query target (volume. mute, or peak meter)
		_Out_ _pKsPropMembHead: a pointer to a PKSPROPERTY_STEPPING_LONG variable for receiving returned channel count information
        _In_ ulBufferSize: a pointer to a ULONG variable that has the size of the buffer pointed by _pKsPropMembHead

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
NTSTATUS CMiniportWaveRT::GetDeviceAttributeSteppings(_In_  ULONG _ulNodeId, _In_  eChannelTargetType _targetType, _Out_ PKSPROPERTY_STEPPING_LONG _pKsPropMembHead, _In_  UINT32 _ui32DataSize)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE ();

    DPF_ENTER(("[CMiniportWaveRT::GetDeviceAttributeSteppings]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);

    switch (_targetType)
    {
        case eVolumeAttribute:
             ntStatus = GetVolumeSteppings(_pKsPropMembHead, _ui32DataSize);;
             break;
        case eMuteAttribute:
             ntStatus = GetMuteSteppings(_pKsPropMembHead, _ui32DataSize);;
             break;
        case ePeakMeterAttribute:
             ntStatus = GetPeakMeterSteppings(_pKsPropMembHead, _ui32DataSize);;
             break;
        default:
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
Exit:
     return ntStatus;
}
#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetDeviceChannelVolume 
 
Decscription:

    When handling GET volume KS property for the device, Portcls calls 
    this method, inside its property handlers, to get the current setting on the specific channel.

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _In_ _uiChannel:  the target channel for this GET volume operation
		_Out_ _pVolume: a pointer to a LONG variable for receiving returned information

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetDeviceChannelVolume(_In_  ULONG _ulNodeId, _In_ UINT32 _uiChannel, _Out_ LONG  *_pVolume)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE ();

    DPF_ENTER(("[CMiniportWaveRT::GetDeviceChannelVolume]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);

    ntStatus = GetChannelVolume(_uiChannel, _pVolume);

Exit:
    return ntStatus;
}

#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::SetDeviceChannelVolume 
 
Decscription:

    When handling SET volume KS property for the device, Portcls calls 
    this method, inside its property handlers, to set the current setting on the specific channel.

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _In_ _uiChannel:  the target channel for this GET volume operation
		_In_ _Volume: volume value to set 

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::SetDeviceChannelVolume(_In_  ULONG _ulNodeId, _In_ UINT32 _uiChannel, _In_  LONG  _Volume)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE ();

    DPF_ENTER(("[CMiniportWaveRT::SetEndpointChannelVolume]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);

    // Snap the volume level to our range of steppings.
    LONG lVolume = VOLUME_NORMALIZE_IN_RANGE(_Volume); 

    ntStatus = SetChannelVolume(_uiChannel, lVolume);

Exit:
    return ntStatus;
}

#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetDeviceChannelPeakMeter 
 
Decscription:

    When handling GET peak meter KS property for the device, Portcls calls 
    this method, inside its property handlers, to get the current setting on the specific channel.

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _In_ _uiChannel:  the target channel for this GET volume operation
        _Out_ _pPeakMeterValue: a pointer to a LONG variable for receiving returned information

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetDeviceChannelPeakMeter(_In_  ULONG _ulNodeId, _In_ UINT32 _uiChannel, _Out_  LONG  *_pPeakMeterValue)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PAGED_CODE ();

    DPF_ENTER(("[CMiniportWaveRT::GetDeviceChannelPeakMeter]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);
    ntStatus = GetChannelPeakMeter(_uiChannel, _pPeakMeterValue);

Exit:
    return ntStatus;
}
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetDeviceChannelMute 
 
Decscription:

    When handling GET mute KS property for the device, Portcls calls 
    this method, inside its property handlers, to get the current setting on the specific channel.

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _In_ _uiChannel:  the target channel for this GET volume operation
		_Out_ _pbMute: a pointer to a BOOL variable for receiving returned information

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS) CMiniportWaveRT::GetDeviceChannelMute(_In_  ULONG _ulNodeId, _In_ UINT32 _uiChannel, _Out_  BOOL  *_pbMute)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE ();

    DPF_ENTER(("[CMiniportWaveRT::GetEndpointChannelMute]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);

    ntStatus = GetChannelMute(_uiChannel, _pbMute);
Exit:
    return ntStatus;
}

#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::SetDeviceChannelMute 
 
Decscription:

    When handling SET mute KS property for the device, Portcls calls 
    this method, inside its property handlers, to set the current setting on the specific channel.

Parameters:

        _In_ _ulNodeId: node id for the target audio engine node
        _In_ _uiChannel:  the target channel for this GET volume operation
		_In_ _bMute: volume value to set 

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

-------------------------------------------------------------------------------------------------------------------------*/
NTSTATUS CMiniportWaveRT::SetDeviceChannelMute(_In_  ULONG _ulNodeId, _In_ UINT32 _uiChannel, _In_  BOOL  _bMute)
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE ();

    DPF_ENTER(("[CMiniportWaveRT::SetEndpointChannelMute]"));
    IF_TRUE_ACTION_JUMP(_ulNodeId != KSNODE_WAVE_AUDIO_ENGINE, ntStatus = STATUS_INVALID_DEVICE_REQUEST, Exit);

    ntStatus = SetChannelMute(_uiChannel, _bMute);

Exit:
    return ntStatus;
}


#pragma code_seg("PAGE")
/*-----------------------------------------------------------------------------
IMiniportAudioEngineNode::GetBufferSizeRange 
 
Decscription:

    Portcls calls this method, inside its property handlers, to get a buffer size capabilities for 
    a specifc format requested at the moment this call was made.

Parameters:

        _In_  ULONG _ulNodeId: the node is for the target audio engine
        _In_  KSDATAFORMAT_WAVEFORMATEX *_pKsDataFormatWfx: the requested format
        _Out_ KSAUDIOENGINE_BUFFER_SIZE_RANGE *_pBufferSizeRange: 

Return Value:

   Appropriate NTSTATUS code

Called at PASSIVE_LEVEL

Remarks

    The purpose of this call is to give an audio client (such as MF SAR or direct WASAPI clients) an idea on the supported buffer size range - 
    almost equivalent to the hardware's buffer capability. An application pick any size within that range that it determines it would best serve
    its purpose (communication apps would like small buffer size for low latency while other media app might choose large buffer for power saving advantage).

-------------------------------------------------------------------------------------------------------------------------*/
NTSTATUS CMiniportWaveRT::GetBufferSizeRange(_In_  ULONG _ulNodeId, _In_ KSDATAFORMAT_WAVEFORMATEX *_pKsDataFormatWfx, _Out_ KSAUDIOENGINE_BUFFER_SIZE_RANGE *_pBufferSizeRange)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE ();
    UNREFERENCED_PARAMETER(_ulNodeId);
    ASSERT(_pBufferSizeRange);
    ASSERT(_pKsDataFormatWfx);
    DPF_ENTER(("[CMiniportWaveRTStream::GetStreamBufferSizeRange]"));

    _pBufferSizeRange->MinBufferBytes = (_pKsDataFormatWfx->WaveFormatEx.nAvgBytesPerSec * MIN_BUFFER_DURATION_MS) / MS_PER_SEC;
    _pBufferSizeRange->MaxBufferBytes = (_pKsDataFormatWfx->WaveFormatEx.nAvgBytesPerSec * MAX_BUFFER_DURATION_MS) / MS_PER_SEC;

    return ntStatus;
}

//------------------------------------------------------------------------------------------------------------------------
//CMiniportWaveRT private supporting functions
//------------------------------------------------------------------------------------------------------------------------
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetVolumeChannelCount(_Out_  UINT32 *_pulChannelCount)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PAGED_CODE ();
    ASSERT (_pulChannelCount);

    DPF_ENTER(("[CMiniportWaveRT::GetVolumeChannelCount]"));
    *_pulChannelCount = m_DeviceMaxChannels;
     return ntStatus;
}
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetVolumeSteppings(_Out_writes_bytes_(_ui32DataSize) PKSPROPERTY_STEPPING_LONG _pKsPropStepLong, _In_  UINT32 _ui32DataSize)
{
    PAGED_CODE ();
    UINT32 ulChannelCount = _ui32DataSize / sizeof(KSPROPERTY_STEPPING_LONG);
    ASSERT (_pKsPropStepLong);
    DPF_ENTER(("[CMiniportWaveRT::GetVolumeSteppings]"));

    ASSERT(ulChannelCount <= m_DeviceMaxChannels);
    if (ulChannelCount > m_DeviceMaxChannels)
    {
        return STATUS_INVALID_PARAMETER;
    }

    for(UINT i = 0; i < ulChannelCount; i++)
    {
        _pKsPropStepLong[i].SteppingDelta = VOLUME_STEPPING_DELTA;
        _pKsPropStepLong[i].Bounds.SignedMaximum = VOLUME_SIGNED_MAXIMUM;
        _pKsPropStepLong[i].Bounds.SignedMinimum = VOLUME_SIGNED_MINIMUM;
    }
    return STATUS_SUCCESS;
}
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetChannelVolume(_In_  UINT32 _uiChannel, _Out_ LONG *_pVolume)
{
    PAGED_CODE ();
    ASSERT (_pVolume);
    DPF_ENTER(("[CMiniportWaveRT::GetChannelVolume]"));

    if (_uiChannel == ALL_CHANNELS_ID)
    {
        *_pVolume = m_plVolumeLevel[0];
    }
    else
    {
        ASSERT(_uiChannel <= m_DeviceMaxChannels);
        *_pVolume = m_plVolumeLevel[_uiChannel];
    }
    return STATUS_SUCCESS;
}
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::SetChannelVolume(_In_  UINT32 _uiChannel, _In_  LONG _Volume)
{
    PAGED_CODE ();
    DPF_ENTER(("[CMiniportWaveRT::SetChannelVolume]"));

    if (_uiChannel == ALL_CHANNELS_ID)
    {
        for (int i=0; i<m_DeviceMaxChannels; i++)
        {
            m_plVolumeLevel[i] = _Volume;
        }
    }
    else
    {
        ASSERT(_uiChannel <= m_DeviceMaxChannels);
        m_plVolumeLevel[_uiChannel] = _Volume;
    }

    return STATUS_SUCCESS;
}
///- metering
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetPeakMeterChannelCount(_Out_  UINT32 *_pulChannelCount)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PAGED_CODE ();
    ASSERT (_pulChannelCount);

    DPF_ENTER(("[CMiniportWaveRT::GetVolumeChannelCount]"));
    *_pulChannelCount = m_DeviceMaxChannels;
     return ntStatus;
}
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetPeakMeterSteppings(_Out_writes_bytes_(_ui32DataSize)  PKSPROPERTY_STEPPING_LONG _pKsPropStepLong, _In_  UINT32 _ui32DataSize)
{
    PAGED_CODE ();
    UINT32 ulChannelCount = _ui32DataSize / sizeof(KSPROPERTY_STEPPING_LONG);

    ASSERT (_pKsPropStepLong);
    DPF_ENTER(("[CMiniportWaveRT::GetVolumeSteppings]"));

    ASSERT(ulChannelCount <= m_DeviceMaxChannels);

    if (ulChannelCount > m_DeviceMaxChannels)
    {
        return STATUS_INVALID_PARAMETER;
    }

    for(UINT i = 0; i < ulChannelCount; i++)
    {
        _pKsPropStepLong[i].SteppingDelta = PEAKMETER_STEPPING_DELTA;
        _pKsPropStepLong[i].Bounds.SignedMaximum = PEAKMETER_SIGNED_MAXIMUM;
        _pKsPropStepLong[i].Bounds.SignedMinimum = PEAKMETER_SIGNED_MINIMUM;
    }
    return STATUS_SUCCESS;
}
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetChannelPeakMeter(_In_  UINT32 _uiChannel, _Out_  LONG *_plPeakMeter)
{
    PAGED_CODE ();
    ASSERT (_plPeakMeter);
    DPF_ENTER(("[CMiniportWaveRT::GetChannelPeakMeter]"));

    ASSERT(_uiChannel < m_DeviceMaxChannels);

    if (_uiChannel >= m_DeviceMaxChannels)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (m_ulSystemAllocated + m_ulOffloadAllocated > 0)
    {
        *_plPeakMeter = PEAKMETER_NORMALIZE_IN_RANGE(PEAKMETER_SIGNED_MAXIMUM / 2);
    }
    else
    {
        *_plPeakMeter = 0;
    }
    //*_plPeakMeter = m_plPeakMeter[lChannel];

    return STATUS_SUCCESS;
}
// mute
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetMuteChannelCount(_Out_  UINT32 *_pulChannelCount)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PAGED_CODE ();
    ASSERT (_pulChannelCount);

    DPF_ENTER(("[CMiniportWaveRT::GetMuteChannelCount]"));
    *_pulChannelCount = m_DeviceMaxChannels;
     return ntStatus;
}
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetMuteSteppings(_Out_writes_bytes_(_ui32DataSize)  PKSPROPERTY_STEPPING_LONG _pKsPropStepLong, _In_  UINT32 _ui32DataSize)
{
    PAGED_CODE ();
    UINT32 ulChannelCount = _ui32DataSize / sizeof(KSPROPERTY_STEPPING_LONG);

    ASSERT (_pKsPropStepLong);
    DPF_ENTER(("[CMiniportWaveRT::GetMuteSteppings]"));

    ASSERT(ulChannelCount <= m_DeviceMaxChannels);

    if (ulChannelCount > m_DeviceMaxChannels)
    {
        return STATUS_INVALID_PARAMETER;
    }

    for(UINT i = 0; i < ulChannelCount; i++)
    {
        _pKsPropStepLong[i].SteppingDelta = 1;
        _pKsPropStepLong[i].Bounds.SignedMaximum = TRUE;
        _pKsPropStepLong[i].Bounds.SignedMinimum = FALSE;
    }
    return STATUS_SUCCESS;
}
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::GetChannelMute(_In_  UINT32 _uiChannel, _Out_  BOOL *_pbMute)
{
    PAGED_CODE ();
    ASSERT (_pbMute);
    DPF_ENTER(("[CMiniportWaveRT::GetChannelMute]"));


    if (_uiChannel == ALL_CHANNELS_ID)
    {
        *_pbMute = m_pbMuted[0];
    }
    else
    {
        ASSERT(_uiChannel <= m_DeviceMaxChannels);
        *_pbMute = m_pbMuted[_uiChannel];
    }

    return STATUS_SUCCESS;
}
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRT::SetChannelMute(_In_  UINT32 _uiChannel, _In_  BOOL _bMute)
{
    PAGED_CODE ();
    DPF_ENTER(("[CMiniportWaveRT::SetChannelVolume]"));

    if (_uiChannel == ALL_CHANNELS_ID)
    {
        for (int i=0; i<m_DeviceMaxChannels; i++)
        {
            m_pbMuted[i] = _bMute;
        }
    }
    else
    {
        ASSERT(_uiChannel <= m_DeviceMaxChannels);
        m_pbMuted[_uiChannel] = _bMute;
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
CMiniportWaveRT::SetLoopbackProtection
(
    _In_ CONSTRICTOR_OPTION ulProtectionOption
)
/*++

Routine Description:

  Allows or disallows loopback streaming.

Arguments:

  Parameters:
    _In_ protectionOption: protection option
                                 CONSTRICTOR_OPTION_DISABLE: Turn protection  off.
                                 CONSTRICTOR_OPTION_MUTE: Mute the loopback stream

Return Value:

  NT status code.

--*/
{
    PAGED_CODE ();
    DPF_ENTER(("[CMiniportWaveRT::SetLoopbackProtection]"));
    
    NTSTATUS    ntStatus    = STATUS_SUCCESS;

    if (ulProtectionOption == m_LoopbackProtection)
    {
        goto Done;
    }

    for (ULONG i = 0; i < MAX_OUTPUT_LOOPBACK_STREAMS; i++)
    {
        if (m_LoopbackStreams[i])
        {
            ntStatus = m_LoopbackStreams[i]->SetLoopbackProtection(ulProtectionOption);
            if (!NT_SUCCESS(ntStatus))
            {
                break;
            }
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        m_LoopbackProtection = ulProtectionOption;
    }
    else 
    {
        //
        // Something went wrong, restore old protection setting.
        //
        for (ULONG i = 0; i < MAX_OUTPUT_LOOPBACK_STREAMS; i++)
        {
            if (m_LoopbackStreams[i])
            {
                (void)m_LoopbackStreams[i]->SetLoopbackProtection(m_LoopbackProtection);
            }
        }
    }

Done:
    return ntStatus;
}


