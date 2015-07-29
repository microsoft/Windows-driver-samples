/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        ExtendedPhotoMode.h

    Abstract:

        This file contains the helper class CExtendedPhotoMode.
        The class is provides initialization and accessor functions for
        KSCAMERA_EXTENDEDPROP_PHOTOMODE. 

    History:

        created 11/20/2014

**************************************************************************/

//
//  Helper class for Extended Photo Mode.
//
class CExtendedPhotoMode
    : public CExtendedHeader
{
public:
    KSCAMERA_EXTENDEDPROP_PHOTOMODE m_PhotoMode;

public:
    //  Standardized ctors to help ensure a consistant definition.
    CExtendedPhotoMode( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
        : CExtendedHeader( flags, result )
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_PhotoMode, sizeof(m_PhotoMode) );
    }

    CExtendedPhotoMode( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : CExtendedHeader(hdr)
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_PhotoMode, sizeof(m_PhotoMode) );
    }

    //  Access functions let you more easily fetch the embedded field.
    ULONG &
    RequestedHistoryFrames()
    {
        return m_PhotoMode.RequestedHistoryFrames;
    }

    ULONG &
    MaxHistoryFrames()
    {
        return m_PhotoMode.MaxHistoryFrames;
    }

    ULONG &
    SubMode()
    {
        return m_PhotoMode.SubMode;
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() &&
               (Size    >= sizeof(*this)) ;
    }
};

