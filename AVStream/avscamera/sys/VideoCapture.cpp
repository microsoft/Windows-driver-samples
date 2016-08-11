/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        videocapture.cpp

    Abstract:

        Video Capture Pin implementation.  

        Handles construction and defines pin data ranges.

    History:

        created 3/8/2001

**************************************************************************/

#include "Common.h"
#include "ntintsafe.h"

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CVideoCapturePin::CVideoCapturePin(IN PKSPIN Pin) :
    CCapturePin (Pin)
{
    PAGED_CODE();

}


/*************************************************

Routine Description:

    Create a new capture pin.  This is the creation dispatch for
    the video capture pin.

Arguments:

    Pin -
        The pin being created

    Irp -
        The creation Irp

Return Value:

    Success / Failure

**************************************************/

NTSTATUS 
CVideoCapturePin::
DispatchCreate(
    IN PKSPIN Pin, 
    IN PIRP Irp
)
{
    PAGED_CODE();

    DBG_ENTER("(Pin=%d)", Pin->Id);

    NTSTATUS Status = STATUS_SUCCESS;

    CCaptureFilter* pFilter = reinterpret_cast <CCaptureFilter*>(KsPinGetParentFilter(Pin)->Context);
    CVideoCapturePin *CapPin = new (NonPagedPoolNx) CVideoCapturePin (Pin);

    if( !CapPin )
    {
        // Fail if we couldn't create the pin.
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = 
            CapPin->Initialize();
    }

    if( NT_SUCCESS (Status) )
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

//
// CapturePinDispatch:
//
// This is the dispatch table for the capture pin.  It provides notifications
// about creation, closure, processing, data formats, etc...
//
DEFINE_CAMERA_KSPIN_DISPATCH( VideoCapturePinDispatch, CVideoCapturePin );

//
// CapturePinAllocatorFraming:
//
// This is the simple framing structure for the capture pin.  Note that this
// will be modified via KsEdit when the actual capture format is determined.
//
DECLARE_SIMPLE_FRAMING_EX(
    VideoCapturePinAllocatorFraming,
    STATICGUIDOF( KSMEMORY_TYPE_KERNEL_NONPAGED ),
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    2,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE
);


