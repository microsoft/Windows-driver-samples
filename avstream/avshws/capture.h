/**************************************************************************

    AVStream Simulated Hardware Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        capture.h

    Abstract:

        This file contains header for the video capture pin on the capture
        filter.  The capture sample performs "fake" DMA directly into
        the capture buffers.  Common buffer DMA will work slightly differently.

        For common buffer DMA, the general technique would be DPC schedules
        processing with KsPinAttemptProcessing.  The processing routine grabs
        the leading edge, copies data out of the common buffer and advances.
        Cloning would not be necessary with this technique.  It would be 
        similiar to the way "AVSSamp" works, but it would be pin-centric.

    History:

        created 3/8/2001

**************************************************************************/
#include <initguid.h>
//
// STREAM_POINTER_CONTEXT:
//
// This is the context structure we associate with all clone stream pointers.
// It allows the mapping code to rip apart the buffer into chunks the same
// size as the scatter/gather mappings in order to fake scatter / gather
// bus-master DMA.
//
typedef struct _STREAM_POINTER_CONTEXT {
    
    PUCHAR BufferVirtual;

} STREAM_POINTER_CONTEXT, *PSTREAM_POINTER_CONTEXT;

//
// CCapturePin:
//
// The video capture pin class.
//
class CCapturePin :
    public ICaptureSink {

private:

    //
    // The AVStream pin we're associated with.
    //
    PKSPIN m_Pin;

    //
    // Pointer to the internal device object for our capture device.
    // We access the "fake" hardware through this object.
    //
    CCaptureDevice *m_Device;
    
    //
    // The state we've put the hardware into.  This allows us to keep track
    // of whether to do things like unpausing or restarting.
    //
    HARDWARE_STATE m_HardwareState;

    //
    // The clock we've been assigned.  As with other capture filters, we do
    // not expose a clock.  If one has been assigned, we will use it to
    // time stamp packets (plus a reasonable delta to work the capture stream
    // in a preview graph).
    //
    PIKSREFERENCECLOCK m_Clock;

    //
    // The captured video info header.  The settings for "fake" hardware will be
    // programmed via this video info header.
    //
    PKS_VIDEOINFOHEADER m_VideoInfoHeader;

    //
    // If we are unable to insert all of the mappings in a stream pointer into
    // the "fake" hardware's scatter / gather table, we set this to the
    // stream pointer that's incomplete.  This is done both to make the 
    // relasing easier and to make it easier to fake the scatter / gather
    // hardware.
    //
    PKSSTREAM_POINTER m_PreviousStreamPointer;

    //
    // An indication of whether or not we pended I/O for some reason.  If this
    // is set, the DPC will resume I/O when any mappings are completed.
    //
    BOOLEAN m_PendIo;

    //
    // An indication of whether or not this pin has acquired the necessary
    // hardware resources to operate.  When the pin reaches KSSTATE_ACQUIRE,
    // we attempt to acquire the hardware.  This flag will be set based on
    // our success / failure.
    //
    BOOLEAN m_AcquiredResources;
    
    //
    // Presentation time for the sample
    //
    LONGLONG m_PresentationTime;

    LONGLONG m_FrameNumber;
    LONGLONG m_DroppedFrames;

    //
    // CleanupReferences():
    //
    // Clean up any references we hold on frames in the queue.  This is called
    // when we abruptly stop the fake hardware.
    //
    NTSTATUS
    CleanupReferences (
        );

    //
    // SetState():
    //
    // This is the state transition handler for the capture pin.  It attempts
    // to acquire resources for the capture pin (or releasing them if
    // necessary) and starts and stops the hardware as required.
    //
    NTSTATUS
    SetState (
        IN KSSTATE ToState,
        IN KSSTATE FromState
        );

    //
    // Process():
    //
    // This is the processing dispatch for the capture pin.  It handles
    // programming the scatter / gather tables for the hardware as buffers
    // become available.  This processing routine is designed for a direct
    // into the capture buffers kind of DMA as opposed to common-buffer
    // and copy strategies.
    //
    NTSTATUS
    Process (
        );
    //
    // CaptureVideoInfoHeader():
    //
    // This routine stashes the video info header set on the pin connection
    // in the CCapturePin object.  This is used to base hardware settings.
    //
    PKS_VIDEOINFOHEADER
    CaptureVideoInfoHeader (
        );

    //
    // Cleanup():
    //
    // This is the free callback from the bagged item (CCapturePin).  If we
    // do not provide a callback when we bag the CCapturePin, ExFreePool
    // would be called.  This is not desirable for C++ constructed objects.
    // We merely delete the object here.
    //
    static
    void
    Cleanup (
        IN CCapturePin *Pin
        )
    {
        delete Pin;
    }

public:
    //
    // CCapturePin():
    //
    // The capture pin's constructor.  Initialize any non-0, non-NULL fields
    // (since new will have zero'ed the memory anyway) and set up our
    // device level pointers for access during capture routines.
    //
    CCapturePin (
        IN PKSPIN Pin
        );

    //
    // ~CCapturePin():
    //
    // The capture pin's destructor.
    //
    ~CCapturePin (
        )
    {
    }

    //
    // ICaptureSink::CompleteMappings()
    //
    // This is the capture sink notification mechanism for mapping completion.
    // When the device DPC detects that a given number of mappings have been
    // completed by the fake hardware, it signals the capture sink of this
    // through this method.
    //
    virtual
    void
    CompleteMappings (
        IN ULONG NumMappings
        );

    /*************************************************

        Dispatch Routines

    *************************************************/

    //
    // DispatchCreate():
    //
    // This is the creation dispatch for the capture pin.  It creates
    // the CCapturePin object and associates it with the AVStream object
    // bagging it in the process.
    //
    static
    NTSTATUS
    DispatchCreate (
        IN PKSPIN Pin,
        IN PIRP Irp
        );

    //
    // DispatchSetState():
    //
    // This is the set device state dispatch for the pin.  The routine bridges
    // to SetState() in the context of the CCapturePin.
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
    // DispatchSetFormat():
    //
    // This is the set data format dispatch for the pin.  This will be called
    // BEFORE pin creation to validate that a data format selected is a match
    // for the range pulled out of our range list.  It will also be called
    // for format changes.
    //
    // If OldFormat is NULL, this is an indication that it's the initial
    // call and not a format change.  Even fixed format pins get this call
    // once.
    //
    static
    NTSTATUS
    DispatchSetFormat (
        IN PKSPIN Pin,
        IN PKSDATAFORMAT OldFormat OPTIONAL,
        IN PKSMULTIPLE_ITEM OldAttributeList OPTIONAL,
        IN const KSDATARANGE *DataRange,
        IN const KSATTRIBUTE_LIST *AttributeRange OPTIONAL
        );

    //
    // DispatchProcess():
    //
    // This is the processing dispatch for the capture pin.  The routine 
    // bridges to Process() in the context of the CCapturePin.
    //
    static 
    NTSTATUS
    DispatchProcess (
        IN PKSPIN Pin
        )
    {
        return 
            (reinterpret_cast <CCapturePin *> (Pin -> Context)) ->
               Process ();
            
    }

    //
    // IntersectHandler():
    //
    // This is the data intersection handler for the capture pin.  This 
    // determines an optimal format in the intersection of two ranges,
    // one local and one possibly foreign.  If there is no compatible format,
    // STATUS_NO_MATCH is returned.
    //
    static
    NTSTATUS
    IntersectHandler (
        IN PKSFILTER Filter,
        IN PIRP Irp,
        IN PKSP_PIN PinInstance,
        IN PKSDATARANGE CallerDataRange,
        IN PKSDATARANGE DescriptorDataRange,
        IN ULONG BufferSize,
        OUT PVOID Data OPTIONAL,
        OUT PULONG DataSize
        );

};