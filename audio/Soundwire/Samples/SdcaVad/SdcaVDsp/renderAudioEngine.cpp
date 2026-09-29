/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    renderaudioengine.cpp

Abstract:

    Render Audio Engine - callbacks for Audio Engine Node

Environment:

    Kernel mode

--*/

#include "private.h"
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>
#include "offloadStreamEngine.h"
#include "SimPeakMeter.h"

#include "TestProperties.h"
#include "AudioFormats.h"

#ifndef __INTELLISENSE__
#include "renderaudioengine.tmh"
#endif

// Sizes for min/max for audioengine buffers
// Buffer duration is for both ping and pong buffers combined
// so multiply it by 2
#define MIN_AUDIOENGINE_BUFFER_DURATION_IN_MS       (10 * 2)
#define MAX_AUDIOENGINE_BUFFER_DURATION_IN_MS       (2000 * 2)

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxAudioEngineRetrieveBufferSizeLimits(
    ACXAUDIOENGINE,
    ACXDATAFORMAT   DataFormat,
    PULONG          MinBufferBytes,
    PULONG          MaxBufferBytes
    )
{
    PAGED_CODE();

    ULONG bytesPerSecond = AcxDataFormatGetAverageBytesPerSec(DataFormat);

    *MinBufferBytes = (ULONG) (MIN_AUDIOENGINE_BUFFER_DURATION_IN_MS * bytesPerSecond / 1000);
    *MaxBufferBytes = (ULONG) (MAX_AUDIOENGINE_BUFFER_DURATION_IN_MS * bytesPerSecond / 1000);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxAudioEngineRetrieveEffectsState(
    ACXAUDIOENGINE  AudioEngine,
    PULONG          State
)
{
    PAGED_CODE();

    PDSP_ENGINE_CONTEXT   pAudioEngineCtx;
    pAudioEngineCtx = GetDspEngineContext(AudioEngine);

    *State = pAudioEngineCtx->GFxEnabled;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxAudioEngineAssignEffectsState(
    ACXAUDIOENGINE  AudioEngine,
    ULONG           State
)
{
    PAGED_CODE();
    
    PDSP_ENGINE_CONTEXT   pAudioEngineCtx;
    pAudioEngineCtx = GetDspEngineContext(AudioEngine);

    pAudioEngineCtx->GFxEnabled = (BOOLEAN)State;
    
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxStreamAudioEngineRetrieveEffectsState(
    ACXSTREAMAUDIOENGINE    StreamAudioEngine,
    PULONG                  State
)
{
    PAGED_CODE();

    PDSP_STREAMAUDIOENGINE_CONTEXT pStreamAudioEngineCtx;
    pStreamAudioEngineCtx = GetDspStreamAudioEngineContext(StreamAudioEngine);

    *State = pStreamAudioEngineCtx->LFxEnabled;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxStreamAudioEngineAssignEffectsState(
    ACXSTREAMAUDIOENGINE    StreamAudioEngine,
    ULONG                   State
)
{
    PAGED_CODE();
    
    PDSP_STREAMAUDIOENGINE_CONTEXT pStreamAudioEngineCtx;
    pStreamAudioEngineCtx = GetDspStreamAudioEngineContext(StreamAudioEngine);

    pStreamAudioEngineCtx->LFxEnabled = (BOOLEAN)State;
    
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxAudioEngineRetrieveEngineMixFormat(
    ACXAUDIOENGINE  AudioEngine,
    ACXDATAFORMAT * Format
    )
{
    PDSP_ENGINE_CONTEXT   audioEngineCtx;
    PAGED_CODE();

    audioEngineCtx = GetDspEngineContext(AudioEngine);

    if (!audioEngineCtx->MixFormat)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    *Format = audioEngineCtx->MixFormat;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxAudioEngineAssignEngineDeviceFormat(
    _In_ ACXAUDIOENGINE AudioEngine,
    _In_ ACXDATAFORMAT  Format
    )
{
    PAGED_CODE();

    // Get the downstream pin
    ACXCIRCUIT parentCircuit = (ACXCIRCUIT)AcxElementGetContainer((ACXELEMENT)AudioEngine);

    ACXPIN downstreamPin = AcxCircuitGetPinById(parentCircuit, DspPinTypeBridge);
    if (!downstreamPin)
    {
        RETURN_NTSTATUS(STATUS_INTERNAL_ERROR);
    }

    // Start by getting the list of formats for the raw mode
    ACXDATAFORMATLIST formatList;
    RETURN_NTSTATUS_IF_FAILED(AcxPinRetrieveModeDataFormatList(downstreamPin, &AUDIO_SIGNALPROCESSINGMODE_RAW, &formatList));

    // Find the format we were given in that list.
    NTSTATUS status = STATUS_NO_MATCH;

    ACX_DATAFORMAT_LIST_ITERATOR formatListIter;
    ACX_DATAFORMAT_LIST_ITERATOR_INIT(&formatListIter);
    AcxDataFormatListBeginIteration(formatList, &formatListIter);

    ACXDATAFORMAT listFormat;
    while (NT_SUCCESS(AcxDataFormatListRetrieveNextFormat(formatList, &formatListIter, &listFormat)))
    {
        if (AcxDataFormatIsEqual(listFormat, Format))
        {
            // Assign the format as the default format.
            // Note there is an existing ACX issue with default format assignment - assigning the default
            // will only work if the format is already in the list (or is the first format added to the list).
            AcxDataFormatListAssignDefaultDataFormat(formatList, listFormat);

            // Use the format we pulled out of our list since it will have an appropriate lifetime
            PDSP_ENGINE_CONTEXT audioEngineCtx;
            audioEngineCtx = GetDspEngineContext(AudioEngine);
            audioEngineCtx->MixFormat = listFormat;

            status = STATUS_SUCCESS;

            break;
        }
    }
    AcxDataFormatListEndIteration(formatList, &formatListIter);

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtPeakMeterRetrieveLevelCallback(
    ACXPEAKMETER    PeakMeter,
    ULONG           Channel,
    LONG *          PeakMeterLevel
    )
{
    PAGED_CODE();

    ASSERT(PeakMeter);

    if (Channel == ALL_CHANNELS_ID)
    {
        Channel = 0;
    }

    PDSP_PEAKMETER_ELEMENT_CONTEXT peakmeterCtx = GetDspPeakMeterElementContext(PeakMeter);
    ASSERT(peakmeterCtx);
    CSimPeakMeter* peakMeter = (CSimPeakMeter *)peakmeterCtx->peakMeter;
    *PeakMeterLevel = peakMeter->GetValue(Channel);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtMuteAssignState(
    ACXMUTE    Mute,
    ULONG      Channel,
    ULONG      State
    )
{
    PDSP_MUTE_ELEMENT_CONTEXT muteCtx;
    ULONG                       i;

    PAGED_CODE();

    muteCtx = GetDspMuteElementContext(Mute);
    ASSERT(muteCtx);

    if (Channel != ALL_CHANNELS_ID)
    {
        muteCtx->MuteState[Channel] = State;
    }
    else
    {
        for (i = 0; i < MAX_CHANNELS; ++i)
        {
            muteCtx->MuteState[i] = State;
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtMuteRetrieveState(
    ACXMUTE   Mute,
    ULONG     Channel,
    ULONG *   State
    )
{
    PDSP_MUTE_ELEMENT_CONTEXT muteCtx;

    PAGED_CODE();

    muteCtx = GetDspMuteElementContext(Mute);
    ASSERT(muteCtx);

    // use first channel for all channels setting.
    if (Channel != ALL_CHANNELS_ID)
    {
        *State = muteCtx->MuteState[Channel];
    }
    else
    {
        *State = muteCtx->MuteState[0];
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtRampedVolumeAssignLevel(
    ACXVOLUME              Volume,
    ULONG                  Channel,
    LONG                   VolumeLevel,
    ACX_VOLUME_CURVE_TYPE,
    ULONGLONG
    )
{
    PDSP_VOLUME_ELEMENT_CONTEXT   volumeCtx;
    ULONG                           i;

    PAGED_CODE();

    volumeCtx = GetDspVolumeElementContext(Volume);
    ASSERT(volumeCtx);

    if (Channel != ALL_CHANNELS_ID)
    {
        volumeCtx->VolumeLevel[Channel] = VolumeLevel;
    }
    else
    {
        for (i = 0; i < MAX_CHANNELS; ++i)
        {
            volumeCtx->VolumeLevel[i] = VolumeLevel;
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtVolumeRetrieveLevel(
    ACXVOLUME Volume,
    ULONG     Channel,
    LONG *    VolumeLevel
)
{
    PDSP_VOLUME_ELEMENT_CONTEXT   volumeCtx;

    PAGED_CODE();

    volumeCtx = GetDspVolumeElementContext(Volume);
    ASSERT(volumeCtx);

    if (Channel != ALL_CHANNELS_ID)
    {
        *VolumeLevel = volumeCtx->VolumeLevel[Channel];
    }
    else
    {
        *VolumeLevel = volumeCtx->VolumeLevel[0];
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxStreamAudioEngineRetrievePresentationPosition(
    _In_    ACXSTREAMAUDIOENGINE    StreamAudioEngine,
    _Out_   PULONGLONG              PositionInBlocks,
    _Out_   PULONGLONG              QPCPosition
)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    ACXSTREAM stream;
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine* streamEngine = NULL;

    PAGED_CODE();

    stream = AcxStreamAudioEngineGetStream(StreamAudioEngine);
    if (stream)
    {
        ctx = GetDspStreamContext(stream);

        streamEngine = static_cast<CStreamEngine*>(ctx->StreamEngine);

        status = streamEngine->GetPresentationPosition(PositionInBlocks, QPCPosition);
    }

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxStreamAudioEngineAssignCurrentWritePosition(
    _In_    ACXSTREAMAUDIOENGINE    StreamAudioEngine,
    _In_    ULONG                   Position
)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    ACXSTREAM stream;
    PDSP_STREAM_CONTEXT ctx;
    COffloadStreamEngine* streamEngine = NULL;

    PAGED_CODE();

    stream = AcxStreamAudioEngineGetStream(StreamAudioEngine);
    if (stream)
    {
        ctx = GetDspStreamContext(stream);

        if (ctx->PinType == DspPinTypeOffload)
        {
            streamEngine = static_cast<COffloadStreamEngine*>(ctx->StreamEngine);

            status = streamEngine->SetCurrentWritePosition(Position);
        }
        else
        {
            status = STATUS_NOT_SUPPORTED;
        }
    }

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxStreamAudioEngineRetrieveLinearBufferPosition(
    _In_    ACXSTREAMAUDIOENGINE    StreamAudioEngine,
    _Out_   PULONGLONG              Position
)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    ACXSTREAM stream;
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine* streamEngine = NULL;

    PAGED_CODE();

    stream = AcxStreamAudioEngineGetStream(StreamAudioEngine);
    if (stream)
    {
        ctx = GetDspStreamContext(stream);

        streamEngine = static_cast<CStreamEngine*>(ctx->StreamEngine);

        status = streamEngine->GetLinearBufferPosition(Position);
    }

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxStreamAudioEngineAssignLastBufferPosition(
    _In_    ACXSTREAMAUDIOENGINE    StreamAudioEngine,
    _In_    ULONG                   Position
)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    ACXSTREAM stream;
    PDSP_STREAM_CONTEXT ctx;
    COffloadStreamEngine* streamEngine = NULL;

    PAGED_CODE();

    stream = AcxStreamAudioEngineGetStream(StreamAudioEngine);
    if (stream)
    {
        ctx = GetDspStreamContext(stream);

        if (ctx->PinType == DspPinTypeOffload)
        {
            streamEngine = static_cast<COffloadStreamEngine*>(ctx->StreamEngine);

            status = streamEngine->SetLastBufferPosition(Position);
        }
        else
        {
            status = STATUS_NOT_SUPPORTED;
        }
    }

    return status;
}

_Use_decl_annotations_
PAGED_CODE_SEG
NTSTATUS
DspR_EvtAcxStreamAudioEngineAssignLoopbackProtection(
    _In_    ACXSTREAMAUDIOENGINE,
    _In_    ACX_CONSTRICTOR_OPTION
)
{
    PAGED_CODE();

    return STATUS_SUCCESS;
}

