/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        ExtendedCameraAngleOffset.h

    Abstract:

        This file contains the helper class CExtendedCameraAngleOffset.
        The class is provides initialization and accessor functions for
        KSCAMERA_EXTENDEDPROP_CAMERAOFFSET. 

    History:

        created 11/26/2014

**************************************************************************/

//
//  Helper class for Extended Camera Angle Offset.
//
class CExtendedCameraAngleOffset
    : public CExtendedHeader
{
public:
    KSCAMERA_EXTENDEDPROP_CAMERAOFFSET  m_Params;

public:
    //  Standardized ctors to help ensure a consistant definition.
    CExtendedCameraAngleOffset( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
        : CExtendedHeader( flags, result )
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_Params, sizeof(m_Params) );
    }

    CExtendedCameraAngleOffset( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : CExtendedHeader(hdr)
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_Params, sizeof(m_Params) );
    }

    //  Access functions let you more easily fetch the embedded field.
    LONG &
    PitchAngle()
    {
        return m_Params.PitchAngle;
    }

    LONG &
    YawAngle()
    {
        return m_Params.YawAngle;
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() &&
               (Size    >= sizeof(*this)) ;
    }
};

