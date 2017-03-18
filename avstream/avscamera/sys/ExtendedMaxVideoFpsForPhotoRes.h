/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        ExtendedMaxVideoFpsForPhotoRes.h

    Abstract:

        This file contains the helper class CExtendedMaxVideoFpsForPhotoRes.
        The class is provides initialization and accessor functions for
        KSCAMERA_MAXVIDEOFPS_FORPHOTORES. 

    History:

        created 11/26/2014

**************************************************************************/

//
//  Helper class for Extended Max Video Fps for Photo Resolution.
//
class CExtendedMaxVideoFpsForPhotoRes
    : public CExtendedHeader
{
public:
    KSCAMERA_MAXVIDEOFPS_FORPHOTORES m_Params;

public:
    //  Standardized ctors to help ensure a consistant definition.
    CExtendedMaxVideoFpsForPhotoRes( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
        : CExtendedHeader( flags, result )
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_Params, sizeof(m_Params) );
    }

    CExtendedMaxVideoFpsForPhotoRes( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : CExtendedHeader(hdr)
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_Params, sizeof(m_Params) );
    }

    //  Access functions let you more easily fetch the embedded field.
    ULONG &
    PhotoResWidth()
    {
        return m_Params.PhotoResWidth;
    }

    ULONG &
    PhotoResHeight()
    {
        return m_Params.PhotoResHeight;
    }

    ULONG &
    PreviewFPSNum()
    {
        return m_Params.PreviewFPSNum;
    }

    ULONG &
    PreviewFPSDenom()
    {
        return m_Params.PreviewFPSDenom;
    }

    ULONG &
    CaptureFPSNum()
    {
        return m_Params.CaptureFPSNum;
    }

    ULONG &
    CaptureFPSDenom()
    {
        return m_Params.CaptureFPSDenom;
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() &&
               (Size    >= sizeof(*this)) ;
    }
};

