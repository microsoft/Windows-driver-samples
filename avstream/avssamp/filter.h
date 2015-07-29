/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        filter.h

    Abstract:

        This file contails the filter level header for the filter-centric
        capture filter.

    History:

        created 5/31/01

**************************************************************************/

/**************************************************************************

    DEFINES

**************************************************************************/

//
// VIDEO_PIN_ID:
//
// The pin factory id of the video pin (the order in the descriptor table).
//
#define VIDEO_PIN_ID 0

/**************************************************************************

    CLASSES

**************************************************************************/

class CCaptureFilter {

private:

    //
    // The AVStream filter object associated with this CCaptureFilter.
    //
    PKSFILTER m_Filter;

    //
    // The DPC used for the timer.
    //
    KDPC m_TimerDpc;
    
    //
    // The timer used for simulation of capture timings.
    //
    KTIMER m_Timer;

    //
    // Boolean used to detect whether the DPC routine is to shutdown or not
    //
    BOOLEAN m_StoppingDPC;

    //
    // The event used to signal successful shutdown of the timer DPC
    //
    KEVENT m_StopDPCEvent;

    //
    // The number of timer ticks that have occurred since the timer DPC 
    // started firing.
    //
    volatile ULONG m_Tick;

    //
    // The system time at the point that the timer DPC starts.
    //
    LARGE_INTEGER m_StartTime;

    //
    // The amount of time between timer DPC's (and hence frame capture
    // triggers).
    //
    LONGLONG m_TimerInterval;

    //
    // The wave object.  This is passed to the audio pin later, but it's
    // used at filter create time to determine what ranges to expose on
    // the audio pin.
    //
    CWaveObject *m_WaveObject;

    //
    // The audio pin factory id.  This is dynamic since the pin is created
    // dynamically at filter create time.
    //
    ULONG m_AudioPinId;

    //
    // Process():
    //
    // The process routine for the capture filter.  This is responsible for
    // copying synthesized data into image buffers.  The DispatchProcess()
    // function bridges to this routine in the context of the CCaptureFilter.
    //
    NTSTATUS
    Process (
        IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
        );


    //
    // BindAudioToWaveObject():
    //
    // This function call binds the audio stream exposed by the filter to
    // the wave object m_WaveObject. 
    //
    NTSTATUS
    BindAudioToWaveObject (
        );

    //
    // Cleanup():
    //
    // This is the bag cleanup callback for the CCaptureFilter.  Not providing
    // one would cause ExFreePool to be used.  This is not good for C++
    // constructed objects.  We simply delete the object here.
    //
    static
    void
    Cleanup (
        IN CCaptureFilter *CapFilter
        )
    {
        delete CapFilter;
    }

public:

    //
    // CCaptureFilter():
    //
    // The capture filter object constructor.  Since the new operator will
    // have zeroed the memory, do not bother initializing any NULL or 0
    // fields.  Only initialize non-NULL, non-0 fields.
    //
    CCaptureFilter (
        IN PKSFILTER Filter
        );

    //
    // ~CCaptureFilter():
    //
    // The capture filter destructor.
    //
    ~CCaptureFilter (
        )
    {
    }

    //
    // StartDPC():
    //
    // This is called in order to start the timer DPC running. 
    //
    void
    StartDPC (
        IN LONGLONG TimerInterval
        );

    //
    // StopDPC():
    //
    // This is called in order to stop the timer DPC running.  The function
    // will not return until it guarantees that no more timer DPC's fire.
    //
    void
    StopDPC (
        );

    //
    // GetWaveObject():
    //
    // Returns the wave object that has been opened for the filter.
    //
    CWaveObject *
    GetWaveObject (
        )
    {
        return m_WaveObject;
    }

    //
    // GetTimerInterval():
    //
    // Returns the timer interval we're using to generate DPC's.
    //
    LONGLONG
    GetTimerInterval (
        );

    /*************************************************

        Dispatch Routines

    *************************************************/

    //
    // DispatchCreate():
    //
    // This is the filter creation dispatch for the capture filter.  It 
    // creates the CCaptureFilter object, associates it with the AVStream
    // object, and bags it for easy cleanup later.
    //
    static
    NTSTATUS
    DispatchCreate (
        IN PKSFILTER Filter,
        IN PIRP Irp
        );

    //
    // DispatchProcess():
    //
    // This is the filter process dispatch for the capture filter.  It merely
    // bridges to Process() in the context of the CCaptureFilter.
    //
    static
    NTSTATUS
    DispatchProcess (
        IN PKSFILTER Filter,
        IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
        )
    {
        return
            (reinterpret_cast <CCaptureFilter *> (Filter -> Context)) ->
                Process (ProcessPinsIndex);
    }


    //
    // TimerDpc():
    //
    // The timer dpc routine.  This is bridged to from TimerRoutine in the
    // context of the appropriate CCaptureFilter.
    //
    void
    TimerDpc (
        );
            
};

