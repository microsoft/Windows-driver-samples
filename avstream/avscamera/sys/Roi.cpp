/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        Roi.cpp

    Abstract:

        This file contains the implemention of CRoiConfig, CRoiProperty and
        related classes used in the filter to implement ROI handling.

    History:

        created 6/27/2013

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//  Helper function to diff ptrs.
inline
UINT_PTR
ByteDiffPtrs( PVOID pBase, PVOID pCurrent )
{
    PAGED_CODE();

    return reinterpret_cast<PBYTE>(pCurrent) - reinterpret_cast<PBYTE>(pBase) ;
}

//  Helper function to get the size of an ROI control based just on the ID.
static
ULONG
GetSizeOfRoiInfo( ULONG ControlId )
{
    PAGED_CODE();

    switch( ControlId )
    {
    case KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE:
        return sizeof(KSCAMERA_EXTENDEDPROP_ROI_WHITEBALANCE);
        break;

    case KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE:
        return sizeof(KSCAMERA_EXTENDEDPROP_ROI_EXPOSURE);
        break;

    case KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE:
        return sizeof(KSCAMERA_EXTENDEDPROP_ROI_FOCUS);
        break;

    default:
        NT_ASSERT(FALSE);
        return 0;
    }
}

//  Helper function to get the size of an ROI control.
static
ULONG
GetSize(
    _In_    PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL pCtrl
)
{
    PAGED_CODE();

    //  This actually needs to return the type based on the ROI_INFO size
    ULONG   RoiInfoSize = GetSizeOfRoiInfo( pCtrl->ControlId );

    if( RoiInfoSize )
    {
        return
            sizeof(KSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL) +
            (pCtrl->ROICount * RoiInfoSize);
    }
    else
    {
        NT_ASSERTMSG("GetSizeOfRoiInfo( pCtrl->ControlId ) returned 0!  Should never happen!", FALSE);
        return 0;
    }
}

//  Helper function to walk to the next ROI control.
static
PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL
NextCtrl(
    _In_    PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL pCtrl
)
{
    PAGED_CODE();

    ULONG   Size = GetSize( pCtrl );

    if( Size )
    {
        return
            reinterpret_cast<PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL>
            (((PBYTE) pCtrl) + Size);
    }
    else
    {
        NT_ASSERTMSG("GetSize( pCtrl ) returned 0!  Should never happen!", FALSE);
        return nullptr;
    }
}

PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL
CRoiProperty::
Find(
    _In_    KSPROPERTY_CAMERACONTROL_EXTENDED_PROPERTY  Property
)
/*++

Routine Description:

    Search this ROI property for a property ID.

Arguments:

    Property -
        Which control to find.

Return Value:

    A pointer to the ROI control structure.

--*/
{
    PAGED_CODE();

    //  We assume the controls have been validated first.
    PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL pIspCtrl =
        reinterpret_cast<PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL> (this+1);

    //  Loop thru the controls.
    for( ULONG i=0; i<m_Hdr.ControlCount; i++ )
    {
        if( pIspCtrl->ControlId == (ULONG) Property )
        {
            return pIspCtrl;
        }

        //  Advance to the next control.
        pIspCtrl = NextCtrl( pIspCtrl );
    }

    return nullptr;
}

PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL
CRoiProperty::
Add(
    _In_    PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL pCtrl
)
/*++

Routine Description:

    Add a control to the end of this header.
    
    Note: This function does not reallocate memory.  Be careful to ensure 
          there is space available.

Arguments:

    pCtrl -
        An ROI control to append.

Return Value:

    A pointer to the appended ROI control structure.

--*/
{
    PAGED_CODE();

    //  Only bother to add if there are ROIs.
    if( pCtrl->ROICount > 0 )
    {
        //  We assume the controls have been validated first.
        PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL pIspCtrl =
            reinterpret_cast<PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL> (this+1);

        //  Loop thru all the controls.
        for( ULONG i=0; i<m_Hdr.ControlCount; i++ )
        {
            //  Advance to the next control.
            pIspCtrl = NextCtrl( pIspCtrl );
        }

        ULONG SizeToCopy = ::GetSize(pCtrl);

        memcpy( pIspCtrl,
                pCtrl,
                SizeToCopy ) ;

        Size += SizeToCopy ;        // Adjust our size.
        m_Hdr.Size += SizeToCopy ;
        m_Hdr.ControlCount++ ;        // Add one to the control count.
    }
    return nullptr;
}

bool
CRoiProperty::
isValid()
/*++

Routine Description:

    Parse the current ROI Property list and make sure all the control 
    structures are self-consistent.

    Note: This is a very non-trivial problem.

Arguments:

    None

Return Value:

    A pointer to the appended ROI control structure.

--*/
{
    PAGED_CODE();

    DBG_ENTER( "()" );

    ULONGLONG   FocusFlags = 0;
    ULONGLONG   ExposureFlags = 0;
    ULONGLONG   WhiteBalanceFlags = 0;

    if( Version < KSCAMERA_EXTENDEDPROP_VERSION ||
            PinId != KSCAMERA_EXTENDEDPROP_FILTERSCOPE ||
            (Capability & ~(KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_CAPS_CANCELLABLE)) != 0 ||
            (Flags & ~KSCAMERA_EXTENDEDPROP_FLAG_CANCELOPERATION) != 0 ||   // Allow cancellation of ROI.
            Size < sizeof(*this) )
    {
        //NT_ASSERT(FALSE);
        if( Version < KSCAMERA_EXTENDEDPROP_VERSION )
        {
            DBG_TRACE( "Failed: Verison=%d", Version );
        }
        if( PinId != KSCAMERA_EXTENDEDPROP_FILTERSCOPE )
        {
            DBG_TRACE( "Failed:  PinId=%d", PinId );
        }
        if( (Capability & ~(KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_CAPS_CANCELLABLE)) != 0 )
        {
            DBG_TRACE( "Failed:  Capability=0x%016llX", Capability );
        }
        if( (Flags & ~KSCAMERA_EXTENDEDPROP_FLAG_CANCELOPERATION) != 0 )
        {
            DBG_TRACE( "Failed:  Flags=0x%016llX", Flags );
        }
        if( Size < sizeof(*this) )
        {
            DBG_TRACE( "Failed(0):  Size=%d, should be at least %Iu", Size, sizeof(*this) );
        }

        DBG_LEAVE( "()=false" );
        return false;
    }

    PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL pIspCtrl =
        reinterpret_cast<PKSCAMERA_EXTENDEDPROP_ROI_ISPCONTROL> (this+1);

    //  Loop thru the controls.
    for( ULONG i=0; i<m_Hdr.ControlCount; i++ )
    {
        //  Make sure there is room to inspect this control
        if( Size < ByteDiffPtrs( this, pIspCtrl+1 ) ||
                m_Hdr.Size < ByteDiffPtrs( &m_Hdr, pIspCtrl+1 ) )
        {
            //NT_ASSERT(FALSE);
            DBG_TRACE( "Failed(1): Size=%d, should be at least %Iu", Size, ByteDiffPtrs( this, pIspCtrl+1 ) );
            DBG_TRACE( "       Or: m_Hdr.Size=%d, should be at least %Iu", m_Hdr.Size, ByteDiffPtrs( &m_Hdr, pIspCtrl+1 ) );
            DBG_LEAVE( "()=false" );
            return false;
        }

        //  Make sure the control is of a known type
        switch( pIspCtrl->ControlId )
        {
        case KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE:
        case KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE:
        case KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE:
            break;

        default:
            //NT_ASSERT(FALSE);
            DBG_LEAVE( "()=false : Invalid ControlId = 0x%08X", pIspCtrl->ControlId );
            return false;
        }

        //  Make sure there is room for the # of ROIs reported
        if( Size < ByteDiffPtrs( this, NextCtrl(pIspCtrl) ) ||
                m_Hdr.Size < ByteDiffPtrs( &m_Hdr, NextCtrl(pIspCtrl) ) )
        {
            //NT_ASSERT(FALSE);
            DBG_TRACE( "Failed(2): Size=%d, should be at least %Iu", Size, ByteDiffPtrs( this, NextCtrl(pIspCtrl) ) );
            DBG_TRACE( "       Or: m_Hdr.Size=%d, should be at least %Iu", m_Hdr.Size, ByteDiffPtrs( &m_Hdr, NextCtrl(pIspCtrl) ) );
            DBG_LEAVE( "()=false" );
            return false;
        }

        //  Validate the ROIs
        for( ULONG j=0; j<pIspCtrl->ROICount; j++ )
        {
            //  Index into to the control's ROI list.  Get the equivilent of "pIspCtrl->RoiInfo[j]"
            PKSCAMERA_EXTENDEDPROP_ROI_INFO pRoiInfo =
                reinterpret_cast<PKSCAMERA_EXTENDEDPROP_ROI_INFO>
                (((PBYTE) (pIspCtrl+1)) + (j * GetSizeOfRoiInfo(pIspCtrl->ControlId) ));

            //  Validate the cooridinates
            if( pRoiInfo->Region.top    < (LONG) TO_Q31(0) ||
                    pRoiInfo->Region.bottom < (LONG) TO_Q31(0) ||
                    pRoiInfo->Region.left   < (LONG) TO_Q31(0) ||
                    pRoiInfo->Region.right  < (LONG) TO_Q31(0) )
            {
                //NT_ASSERT(FALSE);
                DBG_TRACE( "Failed: Region out of range." );
                DBG_TRACE( "        (top=0x%08X, left=0x%08X, bottom=0x%08X, right=0x%08X)"
                           , pRoiInfo->Region.top
                           , pRoiInfo->Region.bottom
                           , pRoiInfo->Region.left
                           , pRoiInfo->Region.right );
                DBG_LEAVE( "()=false" );
                return false;
            }

            //  Validate the RegionOfInterestType
            switch( pRoiInfo->RegionOfInterestType )
            {
            case KSCAMERA_EXTENDEDPROP_ROITYPE_UNKNOWN:
            case KSCAMERA_EXTENDEDPROP_ROITYPE_FACE:
                break;
            default:
                //NT_ASSERT(FALSE);
                DBG_LEAVE( "()=false Failed: RegionOfInterestType=%d", pRoiInfo->RegionOfInterestType );
                return false;
            }

            //  Validate the Weight
            if( pRoiInfo->Weight > 100 )
            {
                //NT_ASSERT(FALSE);
                DBG_LEAVE( "()=false Failed: Weight=%d", pRoiInfo->Weight );
                return false;
            }

            //  Validate the Flags.
            //  Note: the driver doesn't allow mis-matched ROI Flags.
            ULONGLONG   RoiFlags = pRoiInfo->Flags & ~KSCAMERA_EXTENDEDPROP_FLAG_CANCELOPERATION;

            switch( pIspCtrl->ControlId )
            {
            //  Exposure & WhiteBalance only support Auto, Manual & Lock.
            case KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE:
                if( j!=0 )
                {
                    if( RoiFlags != ExposureFlags )
                    {
                        DBG_LEAVE( "()=false : Mismatched Exposure Flags" );
                        return false;
                    }
                }
                else
                {
                    switch( RoiFlags &
                            (KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO|
                             KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK) )
                    {
                    case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO:
                    case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO + KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK:
                    case 0:
                        break;
                    default:
                        DBG_LEAVE( "()=false : Invalid Exposure Flags" );
                        return false;
                    }
                    ExposureFlags = RoiFlags;
                }
                break;

            case KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE:
                if( j!=0 )
                {
                    if( RoiFlags != WhiteBalanceFlags )
                    {
                        DBG_LEAVE( "()=false : Mismatched WhiteBalance Flags" );
                        return false;
                    }
                }
                else
                {
                    switch( RoiFlags &
                            (KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO|
                             KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK) )
                    {
                    case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO:
                    case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO + KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK:
                    case 0:
                        break;
                    default:
                        DBG_LEAVE( "()=false : Invalid WhiteBalance Flags" );
                        return false;
                    }
                    WhiteBalanceFlags = RoiFlags;
                }
                break;

            // Focus supports additional flags...
            case KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE:
                if( j!=0 )
                {
                    if( RoiFlags != FocusFlags )
                    {
                        DBG_LEAVE( "()=false : Mismatched Focus Flags" );
                        return false;
                    }
                }
                else
                {
                    switch( RoiFlags &
                            (KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO   |
                             KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK   |
                             KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS     |
                             KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK) )
                    {
                    case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO:
                    case KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO + KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK:
                    case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS:
                    case KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS + KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK:
                    case 0:
                        break;
                    default:
                        DBG_LEAVE( "()=false : Invalid Focus Flags" );
                        return false;
                    }

                    switch( RoiFlags & KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_MASK )
                    {
                    case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_MACRO:
                    case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_NORMAL:
                    case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE:
                    //  Depreciated cases are not supported:
                    //case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_INFINITY:
                    //case KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_HYPERFOCAL:
                    case 0:
                        break;
                    default:
                        DBG_LEAVE( "()=false : Invalid Focus Range" );
                        return false;
                    }
                    FocusFlags = RoiFlags;
                }
                break;
            }
        }

        //  Advance to the next control.
        pIspCtrl = NextCtrl( pIspCtrl ) ;
    }

    DBG_LEAVE( "()=true" );
    return true;
}

//  Log to the debug output.
void
CRoiProperty::
Log()
{
    PAGED_CODE();

    DBG_TRACE("Version=%d, PinId=%d, Capability=0x%016llX, Flags=0x%016llX, Size=%d",
              Version, PinId, Capability, Flags, Size );

    CRoiIspControl *pIspCtrl = reinterpret_cast<CRoiIspControl *> (this+1);

    //  Loop thru the controls.
    for( ULONG i=0; i<m_Hdr.ControlCount; i++ )
    {
        pIspCtrl->Log();

        //  Advance to the next control.
        pIspCtrl = reinterpret_cast<CRoiIspControl *>( NextCtrl( pIspCtrl ) ) ;
    }
}

//
//  Initialize a block of memory from another ROI control
//
//  Note: This function assumes enough ROI_INFO structures follow
//        such that this copy will not overrun.
//
BOOL
CRoiIspControl::
Init(
    _In_    CRoiProperty *pRoiProperty,
    _In_    KSPROPERTY_CAMERACONTROL_EXTENDED_PROPERTY Property
)
/*++

Routine Description:

    Initialize a block of memory from another ROI control
    
    Note: This function assumes enough ROI_INFO structures follow
          such that this copy will not overrun.

Arguments:

    pRoiProperty -
        The extended ROI control payload.

    Property -
        Which ISP control to copy from the payload.

Return Value:

    TRUE  - The ROI was set.
    FALSE - The ROI was cleared.

--*/
{
    PAGED_CODE();

    CRoiIspControl *
    pCtrl = reinterpret_cast<CRoiIspControl *> (pRoiProperty->Find( Property ));

    if( pCtrl )
    {
        memcpy( this,
                pCtrl,
                pCtrl->GetSize() );
        return TRUE;
    }

    ControlId = Property;
    ROICount = 0;
    return FALSE;
}

//  Log to the debug output.
void
CRoiIspControl::
Log()
{
    PAGED_CODE();

    switch( ControlId )
    {
    case KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE:
        DBG_TRACE("WhiteBalance");
        break;

    case KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE:
        DBG_TRACE("Exposure");
        break;

    case KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE:
        DBG_TRACE("Focus");
        break;

    default:
        DBG_TRACE("[Unknown]");
        NT_ASSERT(FALSE);
        break;
    }

    DBG_TRACE("Result=0x%08X", Result);

    for( ULONG i=0; i<ROICount; i++ )
    {
        //  Index into to the control's ROI list.  Get the equivilent of "pIspCtrl->RoiInfo[j]"
        PKSCAMERA_EXTENDEDPROP_ROI_INFO pRoiInfo =
            reinterpret_cast<PKSCAMERA_EXTENDEDPROP_ROI_INFO>
            (((PBYTE) (this+1)) + (i * GetSizeOfRoiInfo(ControlId) ));

        DBG_TRACE(" | Flags=0x%016llX [Top=0x%08X, Left=0x%08X, Bottom=0x%08X, Right=0x%08X]",
                  pRoiInfo->Flags,
                  pRoiInfo->Region.top,
                  pRoiInfo->Region.left,
                  pRoiInfo->Region.bottom,
                  pRoiInfo->Region.right );
    }
}

//
//  Initialize a block of memory with an empty control list.
//
CRoiIspControl::
CRoiIspControl(
    _In_    KSPROPERTY_CAMERACONTROL_EXTENDED_PROPERTY Property
)
{
    PAGED_CODE();

    ControlId = Property;
    ROICount = 0;
    Result = 0;
    Reserved = 0;
}

ULONG
CRoiIspControl::
GetSize()
{
    PAGED_CODE();

    return (ROICount>0) ? ::GetSize( this ) : 0;
}

