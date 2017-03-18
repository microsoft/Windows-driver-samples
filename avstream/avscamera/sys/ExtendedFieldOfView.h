/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        ExtendedFieldOfView.h

    Abstract:

        This file contains the helper class CExtendedFieldOfView.
        The class is provides initialization and accessor functions for
        KSCAMERA_EXTENDEDPROP_FIELDOFVIEW. 

    History:

        created 11/26/2014

**************************************************************************/

//
//  Helper class for Extended Field of View.
//
class CExtendedFieldOfView
    : public CExtendedHeader
{
public:
    KSCAMERA_EXTENDEDPROP_FIELDOFVIEW   m_Params;

public:
    //  Standardized ctors to help ensure a consistant definition.
    CExtendedFieldOfView( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
        : CExtendedHeader( flags, result )
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_Params, sizeof(m_Params) );
    }

    CExtendedFieldOfView( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : CExtendedHeader(hdr)
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_Params, sizeof(m_Params) );
    }

    //  Access functions let you more easily fetch the embedded field.
    ULONG &
    NormalizedFocalLengthX()
    {
        return m_Params.NormalizedFocalLengthX;
    }

    ULONG &
    NormalizedFocalLengthY()
    {
        return m_Params.NormalizedFocalLengthY;
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() &&
               (Size    >= sizeof(*this)) ;
    }
};

