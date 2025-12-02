/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        PreviewHwSim.cpp

    Abstract:

        This file contains the implementation of the CPreviewHardwareSimulation 
        class.

        This is a specialization of CHardwareSimulation that provides preview-
        specific metadata.

    History:

        created 5/28/2014

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CPreviewHardwareSimulation::CPreviewHardwareSimulation(
    _Inout_ CSensor *Sensor,
    _In_    LONG    PinID
)
    : CHardwareSimulation( Sensor, PinID )
    , m_Illuminated(FALSE)
{
    PAGED_CODE();
}


CPreviewHardwareSimulation::~CPreviewHardwareSimulation()
{
    PAGED_CODE();
}

void
CPreviewHardwareSimulation::
FakeHardware()
/*++

Routine Description:

    Simulate an interrupt and what the hardware would have done in the
    time since the previous interrupt.

Arguments:

    None

Return Value:

    None

--*/
{
    PAGED_CODE();

    //  Do normal timer handling.
    CHardwareSimulation::FakeHardware();

    //  Update the zoom.
    m_Sensor->UpdateZoom();
}

//
//  Helper function that collects the current ISP settings into our metadata 
//  structure.
//
METADATA_PREVIEWAGGREGATION
CPreviewHardwareSimulation::
GetMetadata()
{
    PAGED_CODE();

    METADATA_PREVIEWAGGREGATION Metadata;
    ISP_FRAME_SETTINGS *pSettings = GetIspSettings();

    //  Wipe the metadata so all settings will default to "Not Set".
    RtlZeroMemory(&Metadata, sizeof(Metadata));

    //  IsoGains;
    Metadata.IsoAnalogGain =
        CMetadataSRational(GetRandom((LONG)5000, (LONG)20000), (LONG)10000);  // Some number from 0.5 to 2.0 - for testing.
    Metadata.IsoDigitalGain =
        CMetadataSRational(GetRandom((LONG)5000, (LONG)20000), (LONG)10000);  // Some number from 0.5 to 2.0 - for testing.

    //  WhiteBalanceGains;
    Metadata.WhiteBalanceGain_R =       //  MF_CAPTURE_METADATA_WHITEBALANCE_GAINS
        CMetadataSRational(GetRandom((LONG)5000, (LONG)20000), (LONG)10000);  // Some number from 0.5 to 2.0 - for testing.
    Metadata.WhiteBalanceGain_G =       //  MF_CAPTURE_METADATA_WHITEBALANCE_GAINS
        CMetadataSRational(GetRandom((LONG)5000, (LONG)20000), (LONG)10000);  // Some number from 0.5 to 2.0 - for testing.
    Metadata.WhiteBalanceGain_B =       //  MF_CAPTURE_METADATA_WHITEBALANCE_GAINS
        CMetadataSRational(GetRandom((LONG)5000, (LONG)20000), (LONG)10000);  // Some number from 0.5 to 2.0 - for testing.

    return Metadata;
}


VOID
CPreviewHardwareSimulation::
GetCaptureStats(
    _In_ PKSCAMERA_METADATA_CAPTURESTATS pCaptureStats
    )
{
    PAGED_CODE();

    KSCAMERA_METADATA_CAPTURESTATS CaptureStats = {0};
    ISP_FRAME_SETTINGS *pSettings = GetIspSettings();

    CaptureStats.Flags =
        (KSCAMERA_METADATA_CAPTURESTATS_FLAG_EXPOSURETIME |
        KSCAMERA_METADATA_CAPTURESTATS_FLAG_EXPOSURECOMPENSATION |
        KSCAMERA_METADATA_CAPTURESTATS_FLAG_ISOSPEED |
        KSCAMERA_METADATA_CAPTURESTATS_FLAG_FOCUSSTATE |
        KSCAMERA_METADATA_CAPTURESTATS_FLAG_LENSPOSITION |
        KSCAMERA_METADATA_CAPTURESTATS_FLAG_WHITEBALANCE |
        KSCAMERA_METADATA_CAPTURESTATS_FLAG_FLASH |
        KSCAMERA_METADATA_CAPTURESTATS_FLAG_SENSORFRAMERATE);
    
    //  ExposureTime;
    CaptureStats.ExposureTime = GetCurrentExposureTime();

    //  EVCompensation;
    CaptureStats.ExposureCompensationFlags = pSettings->EVCompensation.Mode;
    CaptureStats.ExposureCompensationValue = pSettings->EVCompensation.Value;

    //  ISOSpeed;
    CaptureStats.IsoSpeed = GetCurrentISOSpeed();
    DBG_TRACE("ISO=%d, ISO Flags=0x%016llX", CaptureStats.IsoSpeed, pSettings->ISOMode);

    //  FocusState;
    m_Sensor->GetFocusState((KSCAMERA_EXTENDEDPROP_FOCUSSTATE *)&CaptureStats.FocusState);

    //  LensPosition;
    CaptureStats.LensPosition = pSettings->FocusSetting.VideoProc.Value.ul;

    //  WhiteBalanceMode;
    CaptureStats.WhiteBalance = (ULONG)pSettings->WhiteBalanceMode;

    //  FlashOn;
    CaptureStats.Flash = (ULONG) pSettings->FlashMode;

    //  SensorFrameRate;
    ULARGE_INTEGER  FrameRate = {0};
    if (m_TimePerFrame)
    {
        FrameRate.LowPart = 60 * 60 * 24;
        FrameRate.HighPart = (ULONG)((ONESECOND * FrameRate.LowPart) / m_TimePerFrame);
    }
    CaptureStats.SensorFramerate = FrameRate.QuadPart;

    if (pCaptureStats)
    {
        *pCaptureStats = CaptureStats;
    }

    return;
}

//
//  Emit metadata here for preview pin.
//
void
CPreviewHardwareSimulation::
EmitMetadata(
    _Inout_ PKSSTREAM_HEADER    pStreamHeader
)
/*++

Routine Description:

    Emit metadata for a photo.

Arguments:

    None

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE();

    NT_ASSERT(pStreamHeader);

    //  Add the normal frame info to the metadata
    //  Note: This call ensures the KSSTREAM_METADATA_INFO is properly initialized.
    CHardwareSimulation::EmitMetadata( pStreamHeader );

    if (0 != (pStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_METADATA))
    {
        PKS_FRAME_INFO          pFrameInfo = (PKS_FRAME_INFO)(pStreamHeader + 1);
        PKSSTREAM_METADATA_INFO pMetadata = (PKSSTREAM_METADATA_INFO) (pFrameInfo + 1);
        ULONG                   BytesLeft = pMetadata->BufferSize - pMetadata->UsedSize;

        if(m_PhotoConfirmationEntry.isRequired())
        {
            if(BytesLeft < sizeof(KSCAMERA_METADATA_PHOTOCONFIRMATION))
            {
                //Fatal driver error, should never come here.
                NT_ASSERT(FALSE);
                return;
            }
            PKSCAMERA_METADATA_PHOTOCONFIRMATION pPhotoConfirmation =
                (PKSCAMERA_METADATA_PHOTOCONFIRMATION) (((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize);
            pPhotoConfirmation->Header.MetadataId = MetadataId_PhotoConfirmation;
            pPhotoConfirmation->Header.Size = sizeof(*pPhotoConfirmation);
            pPhotoConfirmation->PhotoConfirmationIndex = m_PhotoConfirmationEntry.getIndex();
            pMetadata->UsedSize += sizeof(*pPhotoConfirmation);
            BytesLeft -= sizeof(*pPhotoConfirmation);

            DBG_TRACE("PhotoConfirmation Header.MetadataId = %u", pPhotoConfirmation->Header.MetadataId);
            DBG_TRACE("PhotoConfirmation Header.Size       = %u", pPhotoConfirmation->Header.Size);
            DBG_TRACE("PhotoConfirmation Index             = %u", pPhotoConfirmation->PhotoConfirmationIndex);
        }
        else
        {
            DBG_TRACE("Normal frame; no photo confirmation metadata.");
        }

        if (BytesLeft >= sizeof(CAMERA_METADATA_PREVIEWAGGREGATION))
        {
            PCAMERA_METADATA_PREVIEWAGGREGATION pPreviewAggregation =
                (PCAMERA_METADATA_PREVIEWAGGREGATION)(((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize);
            pPreviewAggregation->Header.MetadataId = (ULONG)MetadataId_Custom_PreviewAggregation;
            pPreviewAggregation->Header.Size = sizeof(*pPreviewAggregation);
            pPreviewAggregation->Data = GetMetadata();
            pMetadata->UsedSize += sizeof(*pPreviewAggregation);
            BytesLeft -= sizeof(*pPreviewAggregation);
        }

        if( BytesLeft >= sizeof(KSCAMERA_METADATA_CAPTURESTATS) )
        {
            PKSCAMERA_METADATA_CAPTURESTATS pPreviewCaptureStats =
                (PKSCAMERA_METADATA_CAPTURESTATS) (((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize);
            GetCaptureStats(pPreviewCaptureStats);
            pPreviewCaptureStats->Header.MetadataId = (ULONG)MetadataId_CaptureStats;
            pPreviewCaptureStats->Header.Size = sizeof(*pPreviewCaptureStats);
            pMetadata->UsedSize += sizeof(*pPreviewCaptureStats);
            BytesLeft -= sizeof(*pPreviewCaptureStats);
        }

        if (BytesLeft >= sizeof(CAMERA_METADATA_UVC_HEADER))
        {
            PCAMERA_METADATA_UVC_HEADER pPreviewUvcHeader =
                (PCAMERA_METADATA_UVC_HEADER)(((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize);
            pPreviewUvcHeader->Header.MetadataId = (ULONG)MetadataId_UsbVideoHeader;
            pPreviewUvcHeader->Header.Size = sizeof(CAMERA_METADATA_UVC_HEADER);

            pPreviewUvcHeader->Data.StartOfFrameTimestamp.PresentationTimeStamp = 1;
            pPreviewUvcHeader->Data.EndOfFrameTimestamp.SourceClockReference = 100;

            pMetadata->UsedSize += sizeof(CAMERA_METADATA_UVC_HEADER);
            BytesLeft -= sizeof(CAMERA_METADATA_UVC_HEADER);
        }

        if (BytesLeft >= sizeof(CAMERA_METADATA_EXTRINSICS))
        {
            PCAMERA_METADATA_EXTRINSICS pPreviewExtrinsics =
                (PCAMERA_METADATA_EXTRINSICS)(((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize);
            RtlZeroMemory(pPreviewExtrinsics, sizeof(CAMERA_METADATA_EXTRINSICS));

            pPreviewExtrinsics->Header.MetadataId = (ULONG)MetadataId_CameraExtrinsics;
            pPreviewExtrinsics->Header.Size = sizeof(CAMERA_METADATA_EXTRINSICS);

            pMetadata->UsedSize += sizeof(CAMERA_METADATA_EXTRINSICS);
            BytesLeft -= sizeof(CAMERA_METADATA_EXTRINSICS);
        }

        if (BytesLeft >= sizeof(CAMERA_METADATA_INTRINSICS))
        {
            PCAMERA_METADATA_INTRINSICS pPreviewIntrinsics =
                (PCAMERA_METADATA_INTRINSICS)(((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize);
            RtlZeroMemory(pPreviewIntrinsics, sizeof(CAMERA_METADATA_INTRINSICS));

            pPreviewIntrinsics->Header.MetadataId = (ULONG)MetadataId_CameraIntrinsics;
            pPreviewIntrinsics->Header.Size = sizeof(CAMERA_METADATA_INTRINSICS);

            pMetadata->UsedSize += sizeof(CAMERA_METADATA_INTRINSICS);
            BytesLeft -= sizeof(CAMERA_METADATA_INTRINSICS);
        }

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
            (void) m_Sensor->GetIRTorch(&State);

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
                (pPreviewIllumination->Flags==KSCAMERA_METADATA_FRAMEILLUMINATION_FLAG_ON ?"ON":"OFF"));

            pMetadata->UsedSize += sizeof(KSCAMERA_METADATA_FRAMEILLUMINATION);
            BytesLeft -= sizeof(KSCAMERA_METADATA_FRAMEILLUMINATION);
        }

        //  MF_CAPTURE_METADATA_HISTOGRAM
        //
        //  Generate the entire Histogram metadata blob here.
        //
        if( BytesLeft >= sizeof(CAMERA_METADATA_HISTOGRAM) )
        {
            PCAMERA_METADATA_HISTOGRAM pHistogram =
                (PCAMERA_METADATA_HISTOGRAM) (((PBYTE)pMetadata->SystemVa) + pMetadata->UsedSize);
            ULONG               ChannelMask = m_Synthesizer->GetChannelMask();
            CExtendedProperty   Setting;

            m_Sensor->GetHistogram( &Setting );
            if( Setting.Flags & KSCAMERA_EXTENDEDPROP_HISTOGRAM_ON &&
                    ChannelMask != 0 )
            {
                //  Just checking to see if we messed something up here.
                NT_ASSERT( pMetadata->BufferSize >= CExtendedMetadata::METADATA_MAX + sizeof(CAMERA_METADATA_HISTOGRAM) );

                pHistogram->Header.MetadataId = (ULONG) MetadataId_Custom_Histogram;
                pHistogram->Header.Size = sizeof(*pHistogram);

                //  Acquire the data for every channel.
                pHistogram->Data.ChannelMask = ChannelMask;
                pHistogram->Data.Height = m_Height;
                pHistogram->Data.Width = m_Width;
                pHistogram->Data.FourCC = m_Sensor->GetFourCC( m_PinID );
                m_Synthesizer->Histogram( pHistogram->Data.P0Data, pHistogram->Data.P1Data, pHistogram->Data.P2Data );
                pMetadata->UsedSize += sizeof(*pHistogram);
                BytesLeft -= sizeof(*pHistogram);
            }
        }

        CExtendedVidProcSetting FaceDetect;
        m_Sensor->GetFaceDetection(&FaceDetect);

        if( FaceDetect.Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_PREVIEW )
        {
            DBG_TRACE("PREVIEW");
            EmitFaceMetadata(
                pStreamHeader,
                FaceDetect.GetULONG(),
                FaceDetect.Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_ADVANCED_MASK);
        }
    }
}

