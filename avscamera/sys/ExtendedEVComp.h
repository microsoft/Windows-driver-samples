/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        ExtendedEvComp.h

    Abstract:

        This file contains the helper class CExtendedEvCompensation.
        The class is provides initialization and accessor functions for
        KSCAMERA_EXTENDEDPROP_EVCOMPENSATION. 

    History:

        created 6/3/2014

**************************************************************************/

//
//  Helper class for Extended Properties.
//
class CExtendedEvCompensation
    : public CExtendedHeader
{
public:
    KSCAMERA_EXTENDEDPROP_EVCOMPENSATION m_EvComp;

public:
    //  Standardized ctors to help ensure a consistant definition.
    CExtendedEvCompensation( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
        :CExtendedHeader( flags, result )
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_EvComp, sizeof(m_EvComp) );
    }

    CExtendedEvCompensation( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : CExtendedHeader(hdr)
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_EvComp, sizeof(m_EvComp) );
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() &&
               (Size    >= sizeof(*this)) ;
    }

    //  Access operators let you fetch the value from the value field.
    operator LONG()
    {
        return m_EvComp.Value;
    }

    //  Assignment operators let you assign a value to the property.
    CExtendedEvCompensation &
    operator =( LONG x )
    {
        Value() = x;
        return *this;
    }

    CExtendedEvCompensation &
    operator =( const KSCAMERA_EXTENDEDPROP_EVCOMPENSATION &x )
    {
        m_EvComp = x;
        return *this;
    }

    LONG &
    Min()
    {
        return m_EvComp.Min;
    }

    LONG &
    Max()
    {
        return m_EvComp.Max;
    }

    ULONG &
    Mode()
    {
        return m_EvComp.Mode;
    }

    LONG &
    Value()
    {
        return m_EvComp.Value;
    }

    LONG &
    GetLONG()
    {
        return m_EvComp.Value;
    }
};

