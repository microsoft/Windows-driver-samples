/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        Roi.h

    Abstract:

        This file contains the definition of CRoiConfig, CRoiProperty and
        related classes used in the filter to implement ROI handling.

    History:

        created 6/27/2013

**************************************************************************/

const ULONG MAX_ROI = 4;

class CRoiIspControl ;

class CRoiConfigCaps : public KSCAMERA_EXTENDEDPROP_ROI_CONFIGCAPS
{
public:
    CRoiConfigCaps(
        _In_    KSPROPERTY_CAMERACONTROL_EXTENDED_PROPERTY Id,
        _In_    ULONG MaxROI,
        _In_    ULONGLONG   Caps=0
    )
    {
        ControlId = Id;
        MaxNumberOfROIs = MaxROI;
        Capability = Caps;
    }

    static const ULONGLONG FOCUS_CAPS =
        KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO        |
        KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK        |
        KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS          |
        KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_MACRO         |
        KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_NORMAL        | 
        KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;

    static const ULONGLONG EXPOSURE_CAPS =
        KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
        KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK;

    static const ULONGLONG WHITEBALANCE_CAPS =
        KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
        KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK;
};

class CRoiConfigCapsHdr : public KSCAMERA_EXTENDEDPROP_ROI_CONFIGCAPSHEADER
{
    CRoiConfigCaps  FocusCaps;
    CRoiConfigCaps  ExposureCaps;
    CRoiConfigCaps  WhiteBalanceCaps;

public:
    CRoiConfigCapsHdr()
        : FocusCaps(
              KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE,
              MAX_ROI,
              CRoiConfigCaps::FOCUS_CAPS)
        , ExposureCaps(
              KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE,
              MAX_ROI,
              CRoiConfigCaps::EXPOSURE_CAPS )
        , WhiteBalanceCaps(
              KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE,
              MAX_ROI,
              CRoiConfigCaps::WHITEBALANCE_CAPS )
    {
        ConfigCapCount = 3;
        Reserved = 0;
        Size = sizeof(*this);
    }
};

class CRoiConfig : public KSCAMERA_EXTENDEDPROP_HEADER
{
    CRoiConfigCapsHdr    CapsHdr;
public:
    CRoiConfig()
    {
        Version = 1;
        PinId = KSCAMERA_EXTENDEDPROP_FILTERSCOPE;
        Size = sizeof(*this);
        Result = 0;
        Capability = 0;
        Flags = 0;
    }
    CRoiConfig( KSCAMERA_EXTENDEDPROP_HEADER &hdr )
        : KSCAMERA_EXTENDEDPROP_HEADER(hdr)
    {
        Size = sizeof(*this);
    }

    bool isValid()
    {
        return (Version == KSCAMERA_EXTENDEDPROP_VERSION) &&
               (Size    >= sizeof(*this)) ;
    }
};

//
//  A wrapper for the KSCAMERA_EXTENDEDPROP_ROI payload
//
//  The payload extends beyond the end of this class definition.  However the
//  payload is variable in length, so this class definition contains member
//  functions that can be used to access and append items to the payload.
//  
class CRoiProperty : public KSCAMERA_EXTENDEDPROP_HEADER
{
    KSCAMERA_EXTENDEDPROP_ROI_ISPCONTROLHEADER  m_Hdr;

public:
    //  Default ctor - used to init the buffer when returning ROI info.
    CRoiProperty()
    {
        Version = KSCAMERA_EXTENDEDPROP_VERSION;
        PinId = KSCAMERA_EXTENDEDPROP_FILTERSCOPE;
        Size = sizeof(*this);
        Result = 0;
        Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_CAPS_CANCELLABLE;
        Flags = 0;

        m_Hdr.ControlCount = 0;
        m_Hdr.Size = sizeof(m_Hdr);
        m_Hdr.Reserved = 0;
    }

    //  Validate the CRoiProperty
    bool
    isValid();

    //  Get Size attribute.
    ULONG
    GetSize()
    {
        return Size;
    }

    //  Get number of controls.  (1-3)
    ULONG
    GetControlCount()
    {
        return m_Hdr.ControlCount;
    }

    //  Find a given control after the property header.
    PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL
    Find(
        _In_    KSPROPERTY_CAMERACONTROL_EXTENDED_PROPERTY  Property
    );

    //
    //  Add a control to the end of this header.
    //
    //  Note: This function does not reallocate memory.  Be careful to ensure 
    //        there is space available.
    //
    PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL
    Add(
        _In_    PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL pCtrl
    );

    void
    Log();
};

//  Note: No initializer classes are provided for the following DDI
//  structure definitions since there is no practical difference in their
//  definitions.  However the CRoiIspControl::GetSize() method is aware
//  of them and will report the correct size of the structure if there were
//  any difference in size.
//
//    - KSCAMERA_EXTENDEDPROP_ROI_WHITEBALANCE
//    - KSCAMERA_EXTENDEDPROP_ROI_EXPOSURE
//    - KSCAMERA_EXTENDEDPROP_ROI_FOCUS
//
class CRoiIspControl : public KSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL
{
public:
    BOOL
    Init(
        _In_    CRoiProperty *pRoiProperty,
        _In_    KSPROPERTY_CAMERACONTROL_EXTENDED_PROPERTY Property
    );

    CRoiIspControl(
        _In_    KSPROPERTY_CAMERACONTROL_EXTENDED_PROPERTY Property=
            KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE
    );

    void
    Log();

    ULONG
    GetSize();
};

//  Simple classes used by the driver to hold the maximum allowed number of ROIs.
class CWhiteBalanceRoiIspControl : public CRoiIspControl
{
private:
    KSCAMERA_EXTENDEDPROP_ROI_WHITEBALANCE  ROI[MAX_ROI];

public:
    CWhiteBalanceRoiIspControl(
        _In_    CRoiProperty *pRoiProperty
    )
    {
        Init( pRoiProperty, KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE );
    }

    CWhiteBalanceRoiIspControl()
        : CRoiIspControl( KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE )
    {}

    ULONGLONG
    GetFlags()
    {
        return ( ROICount>0 ) ? ROI[0].ROIInfo.Flags : 0;
    }
};

class CExposureRoiIspControl : public CRoiIspControl
{
private:
    KSCAMERA_EXTENDEDPROP_ROI_EXPOSURE  ROI[MAX_ROI];

public:
    CExposureRoiIspControl(
        _In_    CRoiProperty *pRoiProperty
    )
    {
        Init( pRoiProperty, KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE );
    }

    CExposureRoiIspControl()
        : CRoiIspControl( KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE )
    {}

    ULONGLONG
    GetFlags()
    {
        return ( ROICount>0 ) ? ROI[0].ROIInfo.Flags : 0;
    }
};

class CFocusRoiIspControl : public CRoiIspControl
{
private:
    KSCAMERA_EXTENDEDPROP_ROI_FOCUS  ROI[MAX_ROI];

public:
    CFocusRoiIspControl(
        _In_    CRoiProperty *pRoiProperty
    )
    {
        Init( pRoiProperty, KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE );
    }

    CFocusRoiIspControl()
        : CRoiIspControl( KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE )
    {}

    ULONGLONG
    GetFlags()
    {
        return ( ROICount>0 ) ? ROI[0].ROIInfo.Flags : 0;
    }
};

