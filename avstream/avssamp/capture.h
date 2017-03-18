/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        capture.h

    Abstract:

        This file contains the capture pin level header for all capture pins
        on the sample filter.

    History:

        created 5/31/01

**************************************************************************/

class CCapturePin 
{

protected:

    //
    // The clock object associated with this pin.
    //
    PIKSREFERENCECLOCK m_Clock;

    //
    // The AVStream pin object associated with this pin.
    //
    PKSPIN m_Pin;

    //
    // The CCaptureFilter owning this pin.
    //
    CCaptureFilter *m_ParentFilter;

    //
    // The count of dropped frames.  The base class will reset this upon
    // stopping the pin.
    //
    ULONG m_DroppedFrames;

    //
    // The frame number.
    //
    ULONGLONG m_FrameNumber;

    //
    // Notifications as to frame drop.  This is used to incorporate frame
    // drop data into the synthesis.
    //
    ULONG m_NotifyVidDrop;
    ULONG m_NotifyAudDrop;

    //
    // Current state.
    //
    KSSTATE m_State;

public:

    //
    // CCapturePin():
    //
    // Construct a new capture pin.
    //
    CCapturePin (
        IN PKSPIN Pin
        );

    //
    // ~CCapturePin():
    //
    // Destruct a capture pin.  The destructor is virtual because the cleanup
    // code will delete the derived class as a CCapturePin.
    //
    virtual
    ~CCapturePin (
        )
    {
    }

    //
    // ClockAssigned():
    //
    // Determine whether or not there is a clock assigned to the pin.
    //
    BOOLEAN
    ClockAssigned (
        )
    {
        return (m_Clock != NULL);
    }

    //
    // GetTime():
    //
    // Get the time on the clock.  There must be a clock assigned to the pin
    // for this call to work.  Verification should be made through
    // the ClockAssigned() call.
    //
    LONGLONG
    GetTime (
        )
    {
        return m_Clock -> GetTime ();
    }

    //
    // SetState():
    //
    // Called to set the state of the pin.  The base class performs clock
    // handling and calls the appropriate derived method (Run/Pause/Acquire/
    // Stop).
    //
    NTSTATUS
    SetState (
        IN KSSTATE ToState,
        IN KSSTATE FromState
        );

    //
    // Run():
    //
    // Called when a pin transitions to KSSTATE_ACQUIRE by SetState().
    // The derived class can override this to provide any implementation it
    // needs.
    //
    virtual
    NTSTATUS 
    Run (
        IN KSSTATE FromState
        )
    {
        return STATUS_SUCCESS;
    }

    //
    // Pause():
    //
    // Called when a pin transitions to KSSTATE_PAUSE by SetState().
    // The derived class can override this to provide any implementation it
    // needs.
    //
    virtual
    NTSTATUS
    Pause (
        IN KSSTATE FromState
        )
    {
        return STATUS_SUCCESS;
    }

    //
    // Acquire():
    //
    // Called when a pin transitions to KSSTATE_ACQUIRE by SetState().
    // The derived class can override this to provide any implementation it
    // needs.
    //
    virtual
    NTSTATUS
    Acquire (
        IN KSSTATE FromState
        )
    {
        return STATUS_SUCCESS;
    }

    //
    // Stop():
    //
    // Called when a pin transitions to KSSTATE_STOP by SetState().
    // The derived class can override this to provide any implementation it
    // needs.
    //
    virtual
    NTSTATUS
    Stop (
        IN KSSTATE FromState
        )
    {
        return STATUS_SUCCESS;
    }

    //
    // GetState():
    //
    // Return the current state of the pin.
    //
    KSSTATE
    GetState (
        )
    {
        return m_State;
    }

    //
    // CaptureFrame():
    //
    // Called in order to trigger capture of a frame on the given pin.  The
    // filter's "tick" count is passed as a reference to synthesize an
    // appropriate frame.
    //
    virtual
    NTSTATUS
    CaptureFrame (
        IN PKSPROCESSPIN ProcessPin,
        IN ULONG Tick
        ) = 0;

    //
    // QueryFrameDrop():
    //
    // Query the number of dropped frames.
    //
    ULONG
    QueryFrameDrop (
        );

    //
    // NotifyDrops():
    //
    // Notify the pin how many frames have been dropped on all pins.
    //
    void
    NotifyDrops (
        IN ULONG VidDrop,
        IN ULONG AudDrop
        );

    /*************************************************

        Dispatch Functions

    *************************************************/

    //
    // DispatchSetState():
    //
    // This is the set device state dispatch for the pin.  It merely acts
    // as a bridge to SetState() in the context of the CCapturePin associated
    // with Pin.
    //
    static
    NTSTATUS
    DispatchSetState (
        IN PKSPIN Pin,
        IN KSSTATE ToState,
        IN KSSTATE FromState
        )
    {
        return
            (reinterpret_cast <CCapturePin *> (Pin -> Context)) ->
                SetState (ToState, FromState);
    }

    //
    // BagCleanup():
    //
    // This is the free callback for the CCapturePin that we bag.  Normally,
    // ExFreePool would be used, but we must delete instead.  This function
    // will just delete the CCapturePin instead of freeing it.  Because our
    // destructor is virtual, the appropriate derived class destructor will
    // get called.
    //
    static
    void
    BagCleanup (
        IN CCapturePin *This
        )

    {

        delete This;

    }

};
