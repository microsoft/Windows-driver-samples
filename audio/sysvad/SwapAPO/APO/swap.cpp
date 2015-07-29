//
// Swap.cpp -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Description:
//
//  Implementation of SwapSamples
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

#include "SwapAPO.h"

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
void CopyFrames(
    _Out_writes_(u32FrameCount * u32SamplesPerFrame)
        FLOAT32 *pf32OutFrames,
    _In_reads_(u32FrameCount * u32SamplesPerFrame)
        const FLOAT32 *pf32InFrames,
    UINT32 u32FrameCount,
    UINT32 u32SamplesPerFrame )
{
    CopyMemory(pf32OutFrames, pf32InFrames, sizeof(FLOAT32) * u32FrameCount * u32SamplesPerFrame);
}
#pragma AVRT_CODE_END

#pragma AVRT_CODE_BEGIN
void ProcessSwap(
    FLOAT32 *pf32OutputFrames,
    const FLOAT32 *pf32InputFrames,
    UINT32   u32ValidFrameCount,
    UINT32   u32SamplesPerFrame )
{
    UINT32   u32SampleIndex;
    FLOAT32  fSwap32;

    ASSERT_REALTIME();
    ATLASSERT( IS_VALID_TYPED_READ_POINTER(pf32InputFrames) );
    ATLASSERT( IS_VALID_TYPED_WRITE_POINTER(pf32OutputFrames) );

    // loop through samples
    while (u32ValidFrameCount--)
    {
        for (u32SampleIndex=0; u32SampleIndex+1<u32SamplesPerFrame; u32SampleIndex += 2)
        {
            // apply swap
            fSwap32 = *pf32InputFrames;
            *pf32OutputFrames = *(pf32InputFrames + 1);
            pf32OutputFrames++;
            *pf32OutputFrames = fSwap32;
            pf32OutputFrames++;
            pf32InputFrames += 2;
        }
    }
}
#pragma AVRT_CODE_END


#pragma AVRT_CODE_BEGIN
void ProcessSwapScale(
    FLOAT32 *pf32OutputFrames,
    const FLOAT32 *pf32InputFrames,
    UINT32   u32ValidFrameCount,
    UINT32   u32SamplesPerFrame,
    FLOAT32  *pf32Coefficients )
{
    UINT32   u32SampleIndex;
    FLOAT32  fSwap32;

    ASSERT_REALTIME();
    ATLASSERT( IS_VALID_TYPED_READ_POINTER(pf32InputFrames) );
    ATLASSERT( IS_VALID_TYPED_READ_POINTER(pf32OutputFrames) );

    // loop through samples
    while (u32ValidFrameCount--)
    {
        for (u32SampleIndex=0; u32SampleIndex+1<u32SamplesPerFrame; u32SampleIndex += 2)
        {
            // apply swap for each stereo pair and scale

            // save left channel
            fSwap32 = *pf32InputFrames;
            // left output equals right input times 1st coefficient
            *pf32OutputFrames = *(pf32InputFrames + 1) * pf32Coefficients[u32SampleIndex];
            pf32OutputFrames++;
            
            // right output equals left input times 2nd coefficient
            *pf32OutputFrames = fSwap32  * pf32Coefficients[u32SampleIndex+1];      
            pf32OutputFrames++;

            pf32InputFrames += 2;
        }
    }
}
#pragma AVRT_CODE_END


#pragma AVRT_CODE_BEGIN
void ProcessDelay(
    _Out_writes_(u32ValidFrameCount * u32SamplesPerFrame)
        FLOAT32 *pf32OutputFrames,
    _In_reads_(u32ValidFrameCount * u32SamplesPerFrame)
        const FLOAT32 *pf32InputFrames,
    UINT32       u32ValidFrameCount,
    UINT32       u32SamplesPerFrame,
    _Inout_updates_(u32DelayFrames * u32SamplesPerFrame)
        FLOAT32 *pf32DelayBuffer,
    UINT32       u32DelayFrames,
    _Inout_
        UINT32  *pu32DelayIndex )
{
    ASSERT_REALTIME();
    ATLASSERT( IS_VALID_TYPED_READ_POINTER(pf32InputFrames) );
    ATLASSERT( IS_VALID_TYPED_READ_POINTER(pf32OutputFrames) );
    
    if (u32DelayFrames == 0)
    {
        CopyFrames( pf32OutputFrames,
                    pf32InputFrames,
                    u32ValidFrameCount,
                    u32SamplesPerFrame );
    }
    else
    {

        // Invariants:
        // 0 <= (*pu32DelayIndex) < u32DelayFrames
        // pf32OutputFrames[0 ... u32ValidFrameCount * u32SamplesPerFrame - 1] is writable
        // pf32InputFrames[0 ... u32ValidFrameCount * u32SamplesPerFrame - 1] is readable
        while (u32ValidFrameCount > 0)
        {
            // copy either the rest of the input/output buffer,
            // or the rest of the delay buffer,
            // whichever is smaller
            UINT32 framesToCopy = min(u32ValidFrameCount, u32DelayFrames - (*pu32DelayIndex));
            
            // delay => output
#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "this copy to pf32OutputFrames is in-range")
            CopyFrames( pf32OutputFrames,
                        &pf32DelayBuffer[(*pu32DelayIndex) * u32SamplesPerFrame],
                        framesToCopy,
                        u32SamplesPerFrame );
            
            // input => delay
            CopyFrames( &pf32DelayBuffer[(*pu32DelayIndex) * u32SamplesPerFrame],
                        pf32InputFrames,
                        framesToCopy,
                        u32SamplesPerFrame );

            pf32OutputFrames += framesToCopy * u32SamplesPerFrame;
            pf32InputFrames += framesToCopy * u32SamplesPerFrame;
            u32ValidFrameCount -= framesToCopy;
            
            *pu32DelayIndex += framesToCopy;
            if (*pu32DelayIndex == u32DelayFrames)
            {
                *pu32DelayIndex = 0;
            }
        }

    }
}
#pragma AVRT_CODE_END

