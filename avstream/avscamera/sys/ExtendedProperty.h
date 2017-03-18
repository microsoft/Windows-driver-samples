/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        ExtendedProperty.h

    Abstract:

        This file contains the helper class CExtendedProperty and the coommon
        base class CExtendedHeader.

        CExtendedHeader provides initialization and accessor functions for
        KSCAMERA_EXTENDEDPROP_HEADER.

        CExtendedProperty provides initialization and accessor functions for
        KSCAMERA_EXTENDEDPROP_VALUE.  
        
        CExtendedProperty also combines KSCAMERA_EXTENDEDPROP_HEADER and 
        KSCAMERA_EXTENDEDPROP_VALUE into a single object for use in the
        macros and templates used by our filter and sensor classes.

    History:

        created 5/21/2013

**************************************************************************/

//
//  Helper class for Extended Properties.
//
class CExtendedHeader
    : public KSCAMERA_EXTENDEDPROP_HEADER
{
public:
    //  Standardized ctors to help ensure a consistant definition.
    CExtendedHeader( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
    {
        Version = KSCAMERA_EXTENDEDPROP_VERSION;
        PinId   = KSCAMERA_EXTENDEDPROP_FILTERSCOPE;
        Size    = sizeof(*this);
        Result  = result;
        Capability = 0;
        Flags   = flags;
    }

    CExtendedHeader( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : KSCAMERA_EXTENDEDPROP_HEADER(hdr)
    {
        Size = sizeof(*this);
    }

    bool isValid()
    {
        return Version == KSCAMERA_EXTENDEDPROP_VERSION;
    }
};

class CExtendedProperty
    : public CExtendedHeader
{
public:
    KSCAMERA_EXTENDEDPROP_VALUE m_Value;

public:
    //  Standardized ctors to help ensure a consistant definition.
    CExtendedProperty( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
        : CExtendedHeader( flags, result )
    {
        Size = sizeof(*this);
        m_Value.Value.ull = 0;
    }

    CExtendedProperty( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : CExtendedHeader(hdr)
    {
        Size = sizeof(*this);
        m_Value.Value.ull = 0;
    }

    //  Access operators let you fetch the value from the value field.
    operator LONG()
    {
        return m_Value.Value.l;
    }
    operator ULONG()
    {
        return m_Value.Value.ul;
    }
    operator LONGLONG()
    {
        return m_Value.Value.ll;
    }
    operator ULONGLONG()
    {
        return m_Value.Value.ull;
    }
    operator KSCAMERA_EXTENDEDPROP_VALUE()
    {
        return m_Value;
    }

    //  Assignment operators let you assign a value to the property.
    CExtendedProperty &
    operator =( LONG x )
    {
        m_Value.Value.l = x;
        return *this;
    }

    CExtendedProperty &
    operator =( LONGLONG x )
    {
        m_Value.Value.ll = x;
        return *this;
    }

    CExtendedProperty &
    operator =( ULONG x )
    {
        m_Value.Value.ul = x;
        return *this;
    }

    CExtendedProperty &
    operator =( ULONGLONG x )
    {
        m_Value.Value.ull = x;
        return *this;
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() &&
               (Size    >= sizeof(*this)) ;
    }

};

