/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        ExtendedMetadata.h

    Abstract:

        This file contains the helper class CExtendedMetadata.
        The class is provides initialization and accessor functions for
        KSCAMERA_EXTENDEDPROP_METADATAINFO. 

    History:

        created 6/20/2014

**************************************************************************/

//
//  Helper class for Extended Metadata.
//
class CExtendedMetadata
    : public CExtendedHeader
{
public:
    KSCAMERA_EXTENDEDPROP_METADATAINFO  m_Info;

    //  Maximum metadata required ... except for Histogram
    static const ULONG METADATA_MAX=4096;

    //  Standardized ctors to help ensure a consistant definition.
    CExtendedMetadata(
        _In_    ULONG       pinId=0,            // Just so we have a default ctor.
        _In_    ULONG       size=METADATA_MAX,
        _In_    ULONG       align=KSCAMERA_EXTENDEDPROP_MetadataAlignment_32,
        _In_    ULONGLONG   flags=(KSCAMERA_EXTENDEDPROP_METADATA_ALIGNMENTREQUIRED | KSCAMERA_EXTENDEDPROP_METADATA_SYSTEMMEMORY),
        _In_    ULONG       result=STATUS_SUCCESS
    )
        : CExtendedHeader( flags, result )
    {
        PinId   = pinId;
        Size    = sizeof(*this);
        m_Info.BufferAlignment      = align;
        m_Info.MaxMetadataBufferSize= size;
    }

    //  Used to modify the metadata allocation size.
    CExtendedMetadata &
    operator =( ULONG x )
    {
        m_Info.MaxMetadataBufferSize = x;
        return *this;
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() &&
               (Size    == sizeof(*this)) &&
               ((Flags & ~(KSCAMERA_EXTENDEDPROP_METADATA_SYSTEMMEMORY | KSCAMERA_EXTENDEDPROP_METADATA_ALIGNMENTREQUIRED)) == 0) &&    // Only valid flags
               ((m_Info.BufferAlignment-1) & m_Info.BufferAlignment);   // Not more than 1 bit set.
    }

};

