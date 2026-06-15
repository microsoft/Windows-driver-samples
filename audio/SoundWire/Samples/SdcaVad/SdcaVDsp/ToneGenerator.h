/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    ToneGenerator.h

Abstract:

    Declaration of a generic sine wave generator.

--*/
#ifndef _SAMPLE_TONEGENERATOR_H
#define _SAMPLE_TONEGENERATOR_H

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
    double          m_ToneAmplitude;
    double          m_ToneDCOffset;

public:
    __drv_maxIRQL(PASSIVE_LEVEL)
    __declspec(code_seg("PAGE"))
    ToneGenerator();

    __drv_maxIRQL(PASSIVE_LEVEL)
    __declspec(code_seg("PAGE"))
    ~ToneGenerator();

    __drv_maxIRQL(PASSIVE_LEVEL)
    __declspec(code_seg("PAGE"))
    NTSTATUS
    Init
    (
        _In_    DWORD                   ToneFrequency, 
        _In_    double                  ToneAmplitude,
        _In_    double                  ToneDCOffset,
        _In_    double                  ToneInitialPhase,
        _In_    PWAVEFORMATEXTENSIBLE   WfExt
    );

    __drv_maxIRQL(PASSIVE_LEVEL)
    __declspec(code_seg("PAGE"))
    NTSTATUS
    Init
    (
        _In_    DWORD                   ToneFrequency,
        _In_    PWAVEFORMATEXTENSIBLE   WfExt
    );

    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID
    GenerateSine
    (
        _Out_writes_bytes_(BufferLength) BYTE       *Buffer, 
        _In_                             size_t      BufferLength
    );

    __drv_maxIRQL(PASSIVE_LEVEL)
    __declspec(code_seg("PAGE"))
    VOID
    SetMute
    (
        _In_ bool Value
    )
    {
        m_Mute = Value;
    }

private:
    __drv_maxIRQL(DISPATCH_LEVEL)
    #pragma code_seg()
    VOID InitNewFrame
    (
        _Out_writes_bytes_(FrameSize)   BYTE*  Frame, 
        _In_                            DWORD  FrameSize
    );
};

#endif // _SAMPLE_TONEGENERATOR_H
