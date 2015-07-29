/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        capture.cpp

    Abstract:

        This file contains the capture pin implementation for all capture
        pins on the sample filter.

    History:

        created 5/31/01

**************************************************************************/

#include "avssamp.h"

/**************************************************************************

    PAGED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CCapturePin::
CCapturePin (
    IN PKSPIN Pin
    ) :
    m_Pin (Pin),
    m_State (KSSTATE_STOP)

/*++

Routine Description:

    Construct a new capture pin.  Find out the filter associated with this
    pin and stash a pointer to our parent filter.

Arguments:

    Pin -
        The AVStream pin object being created.

Return Value:

    None

--*/

{

    PAGED_CODE();

    PKSFILTER ParentFilter = KsPinGetParentFilter (Pin);

    m_ParentFilter = reinterpret_cast <CCaptureFilter *> (
        ParentFilter -> Context
        );

}

/*************************************************/


NTSTATUS
CCapturePin::
SetState (
    IN KSSTATE ToState,
    IN KSSTATE FromState
    )

/*++

Routine Description:

    Called when the pin is transitioning state.  This is a bridge from
    DispatchSetState in the context of the capture pin.  The function itself
    performs basic clock handling (things that all the derived pins would use)
    and then calls the appropriate method in the derived class.

Arguments:

    FromState -
        The state the pin is transitioning away from

    ToState -
        The state the pin is transitioning towards

Return Value:

    Success / Failure of state transition.

--*/

{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    switch (ToState) {
        
        case KSSTATE_STOP:

            //
            // Reset the dropped frame counter.
            //
            m_DroppedFrames = 0;
            m_FrameNumber = 0;

            //
            // On a transition to stop, the clock will be released.
            //
            if (m_Clock) {
                m_Clock -> Release ();
                m_Clock = NULL;
            }

            Status = Stop (FromState);
            break;

        case KSSTATE_ACQUIRE:

            //
            // On a transition to acqiure (from stop), the pin queries for
            // its assigned clock.  This can be done either here or at the
            // transition to pause.
            //
            if (FromState == KSSTATE_STOP) {

                Status = KsPinGetReferenceClockInterface (
                    m_Pin,
                    &m_Clock
                    );

                if (!NT_SUCCESS (Status)) {
                    m_Clock = NULL;
                }

            }

            Status = Acquire (FromState);
            break;

        case KSSTATE_PAUSE:

            Status = Pause (FromState);
            break;

        case KSSTATE_RUN:

            Status = Run (FromState);
            break;

    }

    if (NT_SUCCESS (Status)) {
        m_State = ToState;
    }

    return Status;

}

/**************************************************************************

    LOCKED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


ULONG
CCapturePin::
QueryFrameDrop (
    )

/*++

Routine Description:

    Return the number of frames which have been dropped on this pin.

Arguments:

    None

Return Value:

    The number of frames which have been dropped on this pin.

--*/

{

    return m_DroppedFrames;

}

/*************************************************/


void
CCapturePin::
NotifyDrops (
    IN ULONG VidDrop,
    IN ULONG AudDrop
    )

/*++

Routine Description:

    Stash the number of dropped frames on each pin in this pin to allow
    this data to be incorporated into any synthesis.

Arguments:

    VidDrop -
        Number of video frames that have been dropped

    AudDrop -
        Number of audio frames that have been dropped

Return Value:

    None

--*/

{

    m_NotifyVidDrop = VidDrop;
    m_NotifyAudDrop = AudDrop;

}
