/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        VideoHwSim.cpp

    Abstract:

        A fake h/w simulation for the video pin.  Provides Video pin specific 
        metadata.

    History:

        created 5/28/2014

**************************************************************************/

#include "Common.h"


CVideoHardwareSimulation::CVideoHardwareSimulation(
    _Inout_ CSensor *Sensor,
    _In_    LONG    PinID
)
    : CHardwareSimulation( Sensor, PinID )
{}


CVideoHardwareSimulation::~CVideoHardwareSimulation()
{}

//
//  Emit metadata here for video pin.
//
void
CVideoHardwareSimulation::
EmitMetadata(
    _Inout_ PKSSTREAM_HEADER    pStreamHeader
)
{
    NT_ASSERT(pStreamHeader);

    //  Add the normal frame info to the metadata
    //  Note: This call ensures the KSSTREAM_METADATA_INFO is properly initialized.
    CHardwareSimulation::EmitMetadata( pStreamHeader );

    if (0 != (pStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_METADATA))
    {
        CExtendedVidProcSetting FaceDetect;
        m_Sensor->GetFaceDetection(&FaceDetect);

        if( FaceDetect.Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_VIDEO )
        {
            DBG_TRACE("VIDEO");
            EmitFaceMetadata(
                pStreamHeader,
                FaceDetect.GetULONG(),
                FaceDetect.Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_ADVANCED_MASK);
        }
    }
}

