/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        YUVSynthesizer.h

    Abstract:

        This file contains the definition of CYUVSynthesizer.

        A Base image synthesizer for all YUV formats.  It provides a YUV color
        palette and sets up cached gradient bars.

    History:

        created 4/14/2014

**************************************************************************/

#pragma once

/*************************************************

    CYUVSynthesizer

    Image synthesizer for YUV format.

*************************************************/


class CYUVSynthesizer :
    public CSynthesizer
{
protected:
    //
    //  GetPalette
    //
    //  Get a pointer to an array of palette colors.  Used mostly to handle
    //  different color primaries.  Location of the primary must agree with
    //  Commit().
    virtual
    const UCHAR4 *
    GetPalette();

    //
    // Initialize()
    //
    //  Initialize the Gradient bmp for YUV color space.
    //
    virtual
    BOOLEAN
    Initialize();

public:

    //Default constructor
    CYUVSynthesizer (
        PCCHAR Name="[Something YUV]",
        ULONG Width=0,
        ULONG Height=0
    )
        : CSynthesizer(Name, CHANNEL_YCrCb, Width, Height)
    {}
};

