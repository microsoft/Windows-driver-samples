/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        PreviewHwSim.h

    Abstract:

        This file contains the definition of the CPreviewHardwareSimulation class.

        This is a specialization of CHardwareSimulation that provides preview-
        specific metadata.

    History:

        created 5/28/2014

**************************************************************************/

#pragma once

//  forward references
struct ISP_FRAME_SETTINGS;

class CPreviewHardwareSimulation :
    public CHardwareSimulation
{
public:
    CPreviewHardwareSimulation(
        _Inout_ CSensor *Sensor,
        _In_    LONG PinID
    );

    virtual ~CPreviewHardwareSimulation();

    virtual
    void
    FakeHardware();

protected:
    METADATA_PREVIEWAGGREGATION
    CPreviewHardwareSimulation::
    GetMetadata();

    //  Inject preview pin-specific metadata.
    virtual
    void
    EmitMetadata(
        _Inout_ PKSSTREAM_HEADER   pStreamHeader
    );

};

