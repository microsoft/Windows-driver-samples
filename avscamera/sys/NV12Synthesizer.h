/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        CNV12Synthesizer.h

    Abstract:

        This file defines a class that implements NV12 image synthesis.
        
        CNV12Synthesis is derived from CYUVSynthesis.  It uses the YUV color
        space and defines a Commit() function that reformats pixels into the 
        NV12 format.  From MSDN:

            A format in which all Y samples are found first in memory as an 
            array of unsigned char with an even number of lines (possibly with 
            a larger stride for memory alignment). This is followed immediately 
            by an array of unsigned char containing interleaved Cb and Cr 
            samples. If these samples are addressed as a little-endian WORD 
            type, Cb would be in the least significant bits and Cr would be in 
            the most significant bits with the same total stride as the Y 
            samples. NV12 is the preferred 4:2:0 pixel format.

        A visual representation of the layout:
            YYYY
            YYYY
            UVUV


    History:
        created 4/16/2013

**************************************************************************/

#pragma once

class CNV12Synthesizer :
    public CYUVSynthesizer
{
public:
    //
    // DEFAULT CONSTRUCTOR
    //
    CNV12Synthesizer (
        ULONG Width=0,
        ULONG Height=0
    ) :
        CYUVSynthesizer("NV12", Width, Height)
    {
        NT_ASSERT( ( Width % 2 ) == 0 );
        NT_ASSERT( ( Height % 2 ) == 0 );
    }

    //
    //  Commit
    //
    //  Copy (and reformat, if necessary) pixels from the internal scratch
    //  buffer.  If the output format decimates chrominance, do it here.
    //
    virtual
    _Success_(return > 0)
    ULONG
    Commit(
        _Out_writes_bytes_(Size)
        PUCHAR  Buffer,
        _In_    ULONG   Size,
        _In_    ULONG   Stride
    );
};