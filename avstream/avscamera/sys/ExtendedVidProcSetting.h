/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        ExtendedProperty.h

    Abstract:

        This file contains the helper class CExtendedVidProcSetting.
        The class is provides initialization and accessor functions for
        KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING. 

    History:

        created 5/21/2013

**************************************************************************/

//
//  Helper class for Extended Properties KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING.
//
class CExtendedVidProcSetting
    : public CExtendedHeader
{
public:
    KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING m_Setting;

public:
    //  Standardized ctors to help ensure a consistant definition.
    CExtendedVidProcSetting( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
        : CExtendedHeader( flags, result )
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_Setting, sizeof(m_Setting) );
    }

    CExtendedVidProcSetting( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : CExtendedHeader(hdr)
    {
        Size = sizeof(*this);
        RtlZeroMemory( &m_Setting, sizeof(m_Setting) );
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() &&
               (Size    >= sizeof(*this)) ;
    }

    template<class T>
    NTSTATUS
    BoundsCheck( T Value )
    {
        return ::BoundsCheck( Value, m_Setting );
    }

    //  Access operators let you fetch the value from the value field.
    operator LONG()
    {
        return m_Setting.VideoProc.Value.l;
    }
    operator ULONG()
    {
        return m_Setting.VideoProc.Value.ul;
    }
    operator LONGLONG()
    {
        return m_Setting.VideoProc.Value.ll;
    }
    operator ULONGLONG()
    {
        return m_Setting.VideoProc.Value.ull;
    }

    //  Assignment operators let you assign a value to the property.
    CExtendedVidProcSetting &
    operator =( LONG x )
    {
        m_Setting.VideoProc.Value.l = x;
        return *this;
    }

    CExtendedVidProcSetting &
    operator =( LONGLONG x )
    {
        m_Setting.VideoProc.Value.ll = x;
        return *this;
    }

    CExtendedVidProcSetting &
    operator =( ULONG x )
    {
        m_Setting.VideoProc.Value.ul = x;
        return *this;
    }

    CExtendedVidProcSetting &
    operator =( ULONGLONG x )
    {
        m_Setting.VideoProc.Value.ull = x;
        return *this;
    }

    CExtendedVidProcSetting &
    operator =( const KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING &x )
    {
        m_Setting = x;
        return *this;
    }

    LONG &
    Min()
    {
        return m_Setting.Min;
    }

    LONG &
    Max()
    {
        return m_Setting.Max;
    }

    LONG &
    Step()
    {
        return m_Setting.Step;
    }

    ULONG &
    Mode()
    {
        return m_Setting.Mode;
    }

    LONG &
    GetLONG()
    {
        return m_Setting.VideoProc.Value.l;
    }

    LONGLONG &
    GetLONGLONG()
    {
        return m_Setting.VideoProc.Value.ll;
    }

    ULONG &
    GetULONG()
    {
        return m_Setting.VideoProc.Value.ul;
    }

    ULONGLONG &
    GetULONGLONG()
    {
        return m_Setting.VideoProc.Value.ull;
    }

};

