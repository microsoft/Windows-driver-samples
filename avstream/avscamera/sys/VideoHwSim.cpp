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
    , m_Illuminated(FALSE)
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
        PKS_FRAME_INFO          pFrameInfo = (PKS_FRAME_INFO)(pStreamHeader + 1);
        PKSSTREAM_METADATA_INFO pMetadata = (PKSSTREAM_METADATA_INFO)(pFrameInfo + 1);
        ULONG                   BytesLeft = pMetadata->BufferSize - pMetadata->UsedSize;

        //  TODO: This metadata should only be exposed on a sensor category preview pin.
        //        It's possible I should derive a new preview sim to handle this; but for now
        //        we just populate the IR illumination state on every preview pin.  Hopefully 
        //        the app is smart enough to realize that this is meaningless if they have not 
        //        selected an IR mediatype.
        if (BytesLeft >= sizeof(KSCAMERA_METADATA_FRAMEILLUMINATION))
        {
            PKSCAMERA_METADATA_FRAMEILLUMINATION pPreviewIllumination =
                (PKSCAMERA_METADATA_FRAMEILLUMINATION)(((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize);
            RtlZeroMemory(pPreviewIllumination, sizeof(KSCAMERA_METADATA_FRAMEILLUMINATION));

            pPreviewIllumination->Header.MetadataId = (ULONG)MetadataId_FrameIllumination;
            pPreviewIllumination->Header.Size = sizeof(KSCAMERA_METADATA_FRAMEILLUMINATION);

            //  Toggle the Illumination state for an IR frame.
            CExtendedVidProcSetting State;
            (void)m_Sensor->GetIRTorch(&State);

            if (State.Flags & KSCAMERA_EXTENDEDPROP_IRTORCHMODE_ALWAYS_ON)
            {
                m_Illuminated = TRUE;
                pPreviewIllumination->Flags = KSCAMERA_METADATA_FRAMEILLUMINATION_FLAG_ON;
            }
            else if ((State.Flags & KSCAMERA_EXTENDEDPROP_IRTORCHMODE_ALTERNATING_FRAME_ILLUMINATION))
            {
                m_Illuminated = !m_Illuminated;
                if (m_Illuminated)
                {
                    pPreviewIllumination->Flags = KSCAMERA_METADATA_FRAMEILLUMINATION_FLAG_ON;
                }
            }
            else
            {
                m_Illuminated = FALSE;
            }

            DBG_TRACE("Frame Illumination: Flags=0x%016llX, State=%s",
                State.Flags,
                (pPreviewIllumination->Flags == KSCAMERA_METADATA_FRAMEILLUMINATION_FLAG_ON ? "ON" : "OFF"));

            pMetadata->UsedSize += sizeof(KSCAMERA_METADATA_FRAMEILLUMINATION);
            BytesLeft -= sizeof(KSCAMERA_METADATA_FRAMEILLUMINATION);
        }

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

