/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    ToneGenerator.h

Abstract:

    Declaration of SYSVAD sine wave generator.


--*/
#ifndef _SYSVAD_TONEGENERATOR_H
#define _SYSVAD_TONEGENERATOR_H

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits.h>

class ToneGenerator
{
public:
    DWORD           m_Frequency; 
    WORD            m_ChannelCount; 
    WORD            m_BitsPerSample;
    DWORD           m_SamplesPerSecond;
    double          m_Theta;
    double          m_SampleIncrement;  
    bool            m_Mute;
    BYTE*           m_PartialFrame;
    DWORD           m_PartialFrameBytes;
    DWORD           m_FrameSize;

public:
    ToneGenerator();
    ~ToneGenerator();
    
    NTSTATUS
    Init
    (
        _In_    DWORD                   ToneFrequency, 
        _In_    PWAVEFORMATEXTENSIBLE   WfExt
    );
    
    VOID 
    GenerateSine
    (
        _Out_writes_bytes_(BufferLength) BYTE       *Buffer, 
        _In_                             size_t      BufferLength
    );

    VOID
    SetMute
    (
        _In_ bool Value
    )
    {
        m_Mute = Value;
    }

private:
    VOID InitNewFrame
    (
        _Out_writes_bytes_(FrameSize)   BYTE*  Frame, 
        _In_                            DWORD  FrameSize
    );
};

#endif // _SYSVAD_TONEGENERATOR_H

