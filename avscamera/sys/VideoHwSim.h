/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        VideoHwSim.h

    Abstract:

        A fake h/w simulation for the video pin.  Provides Video pin specific 
        metadata.

    History:

        created 5/28/2014

**************************************************************************/

#pragma once
class CVideoHardwareSimulation :
    public CHardwareSimulation
{
public:
    CVideoHardwareSimulation(
        _Inout_ CSensor *Sensor,
        _In_    LONG PinID
    );

    virtual ~CVideoHardwareSimulation();

    //
    //  Emit metadata here for video pin.
    //
    void
    EmitMetadata(
        _Inout_ PKSSTREAM_HEADER    pStreamHeader
    );
};

