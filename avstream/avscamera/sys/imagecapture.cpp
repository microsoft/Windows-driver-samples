/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        imagecapture.cpp

    Abstract:

        An implementation of CImageCapturePin

    History:

        created 3/8/2001

**************************************************************************/

#include "Common.h"

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

CImageCapturePin::
CImageCapturePin(
    _In_    PKSPIN Pin
)   : CCapturePin (Pin)
{
    PAGED_CODE();
}

NTSTATUS
CImageCapturePin::
DispatchCreate(
    _In_    PKSPIN Pin,
    _In_    PIRP Irp
)
/*++

Routine Description:

    Static member function called when a pin is created.  Instantiate a 
    CImageCapturePin and attach it to our filter object.

Arguments:

    Pin - 
        The KSPIN to wrap.

    Irp -
        The IRP assocated with this request.

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE();

    DBG_ENTER("(Pin=%d)", Pin->Id);

    NTSTATUS Status = STATUS_SUCCESS;

    CCaptureFilter* pFilter = reinterpret_cast <CCaptureFilter*>(KsPinGetParentFilter(Pin)->Context);
    CImageCapturePin *CapPin = new (NonPagedPoolNx) CImageCapturePin (Pin);

    if( !CapPin )
    {
        // Fail if we couldn't create the pin.
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //  Query the filter for the PhotoMode so we can figure out how many frames should be allocated.
        CExtendedPhotoMode  Mode;
        Mode.PinId = Pin->Id;       // Use Pin ID from KS.
        Status = pFilter->GetPhotoMode( &Mode );

        //  CSensor::GetPhotoMode could fail if the framework gives us a Pin ID that doesn't match our descriptors...
        if( NT_SUCCESS(Status) )
        {
            //  Override the number of frames.
            CapPin->SetDesiredFrames( 
                ( (Mode.Flags & KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE) ? 
                    Mode.RequestedHistoryFrames() : 1)
                        + IMAGE_CAPTURE_PIN_MINIMUM_FRAMES );

            Status = 
                CapPin->Initialize();
        }
    }

    if (NT_SUCCESS (Status)) 
    {
        //
        // Adjust the stream header size.  The video packets have extended
        // header info (KS_FRAME_INFO).
        //
        pFilter->setPin(CapPin, Pin->Id);
    }
    else
    {
        //  Clean up.
        delete CapPin;
    }

    DBG_LEAVE("(Pin=%d)=0x%08X", Pin->Id, Status);
    return Status;
}


NTSTATUS
CImageCapturePin::
Close(
    _In_    PIRP Irp
)
{
    PAGED_CODE();

    //  Reset the image sim's counts and queues.
    Reset();

    //  Call the base Close operation.
    return CCapturePin::Close(Irp);
}

// ImageCapturePinDispatch:
//
// This is the dispatch table for the capture pin.  It provides notifications
// about creation, closure, processing, data formats, etc...
// just a copy of CapturePinDispatch for now.
//
DEFINE_CAMERA_KSPIN_DISPATCH( ImageCapturePinDispatch, CImageCapturePin );


//
// ImagePinAllocatorFraming:
//
// This is the simple framing structure for the capture pin.  Note that this
// will be modified via KsEdit when the actual capture format is determined.
// just a copy of CapturePinAllocatorFraming for now.
//
DECLARE_SIMPLE_FRAMING_EX (
    ImageCapturePinAllocatorFraming,
    STATICGUIDOF (KSMEMORY_TYPE_KERNEL_NONPAGED),
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    4,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE
);

