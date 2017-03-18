/**************************************************************************

    AvsCam - An AVStream Simulated Camera Device.

    Copyright (c) 2014, Microsoft Corporation.

    File:

        CameraProfile.h

    Abstract:

        This file contains the helper class CExtendedProfile.

    History:

        created 3/19/2015

**************************************************************************/

//
//  Helper class for CameraProfile
//
class CExtendedProfile : public CExtendedHeader
{
public:
    KSCAMERA_EXTENDEDPROP_PROFILE   m_Profile;

public:
    CExtendedProfile( ULONGLONG flags=0, ULONG result=STATUS_SUCCESS )
        : CExtendedProfile( flags, result )
    {
        Size = sizeof(*this);
        m_Profile.ProfileId = KSCAMERAPROFILE_Legacy;
        m_Profile.Index = 0;
        m_Profile.Reserved = 0;
    }

    CExtendedProfile( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : CExtendedHeader(hdr)
    {
        Size = sizeof(*this);
        m_Profile.ProfileId = KSCAMERAPROFILE_Legacy;
        m_Profile.Index = 0;
        m_Profile.Reserved = 0;
    }

    bool isValid()
    {
        return CExtendedHeader::isValid() && (Size >= sizeof(*this));
    }
};


