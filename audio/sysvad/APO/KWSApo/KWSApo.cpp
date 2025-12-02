//
// KWSApo.cpp -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Description:
//
//  Implementation of ProcessBuffer
//
#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlsync.h>
#include <mmreg.h>

#include <audioenginebaseapo.h>
#include <baseaudioprocessingobject.h>
#include <resource.h>

#include <float.h>

#include "KWSApo.h"

#pragma AVRT_CODE_BEGIN
void WriteSilence(
    _Out_writes_(u32FrameCount * u32SamplesPerFrame)
        FLOAT32 *pf32Frames,
    UINT32 u32FrameCount,
    UINT32 u32SamplesPerFrame )
{
    ZeroMemory(pf32Frames, sizeof(FLOAT32) * u32FrameCount * u32SamplesPerFrame);
}
#pragma AVRT_CODE_END

#pragma AVRT_CODE_BEGIN
void ProcessBuffer(
    FLOAT32 *pf32OutputFrames,
    const FLOAT32 *pf32InputFrames,
    UINT32   u32ValidFrameCount,
    INTERLEAVED_AUDIO_FORMAT_INFORMATION *formatInfo)
{
    UINT32 totalChannelCount = (formatInfo->PrimaryChannelCount + formatInfo->InterleavedChannelCount);

    ASSERT_REALTIME();
    ATLASSERT( IS_VALID_TYPED_READ_POINTER(pf32InputFrames) );
    ATLASSERT( IS_VALID_TYPED_WRITE_POINTER(pf32OutputFrames) );

    // loop through samples
    while (u32ValidFrameCount--)
    {
        // copy over the Primary channel data
        for (UINT32 i = formatInfo->PrimaryChannelStartPosition; i < (formatInfo->PrimaryChannelStartPosition + formatInfo->PrimaryChannelCount); i++)
        {
            *pf32OutputFrames = *(pf32InputFrames + i);
            pf32OutputFrames++;
        }

        // step forward to the next frame, ignoring interleaved data
        pf32InputFrames += (totalChannelCount);
    }
}

#pragma AVRT_CODE_END

