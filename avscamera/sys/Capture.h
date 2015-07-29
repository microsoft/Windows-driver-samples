/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        capture.h

    Abstract:

        This file contains header for the base capture pin class for a filter.  
        The camera sample performs "fake" DMA directly into the capture 
        buffers.  Common buffer DMA will work slightly differently.

        For common buffer DMA, the general technique would be DPC schedules
        processing with KsPinAttemptProcessing.  The processing routine grabs
        the leading edge, copies data out of the common buffer and advances.
        Cloning would not be necessary with this technique.  It would be
        similiar to the way "AVSSamp" works, but it would be pin-centric.

    History:

        created 3/8/2001

**************************************************************************/

#pragma once
#include <initguid.h>

#define DMAX_X 640
#define DMAX_Y 480
#define D_X 320
#define D_Y 240

#define DMAX_X_OS 728
#define DMAX_Y_OS 576
#define D_X_OS     384
#define D_Y_OS     240

///////////////////////////////////////////////////////////////////////////////
// GUID for primary and FFC cameras
///////////////////////////////////////////////////////////////////////////////
// {9D9A0F5E-CDE9-49f5-B464-BF13429DDEB5}
DEFINE_GUID(PRIMARY_CAMERA_GUID,
            0x9d9a0f5e, 0xcde9, 0x49f5, 0xb4, 0x64, 0xbf, 0x13, 0x42, 0x9d, 0xde, 0xb5);

// {75D53C4E-963E-4415-AA88-89107F08D1EE}
DEFINE_GUID(FRONT_FACING_CAMERA_GUID,
            0x75d53c4e, 0x963e, 0x4415, 0xaa, 0x88, 0x89, 0x10, 0x7f, 0x8, 0xd1, 0xee);

/*
#define Mpx12_X 4160
#define Mpx12_Y 3120
#define Mpx5_X  2592
#define Mpx5_Y  1944
#define Mpx8_X  3264
#define Mpx8_Y  2448
*/
//
// STREAM_POINTER_CONTEXT:
//
// This is the context structure we associate with all clone stream pointers.
// It allows the mapping code to rip apart the buffer into chunks the same
// size as the scatter/gather mappings in order to fake scatter / gather
// bus-master DMA.
//
typedef struct _STREAM_POINTER_CONTEXT
{

    PUCHAR BufferVirtual;

} STREAM_POINTER_CONTEXT, *PSTREAM_POINTER_CONTEXT;

//
// CCapturePin:
//
// The video capture pin class.
//
class CCapturePin :
    public ICapturePin,
    public CNonCopyable
{
protected:
    PKSPIN m_Pin;                               // The AVStream pin we're associated with.
    CCaptureDevice *m_Device;                   // Pointer to the internal device object for our capture device.
    CSensor        *m_Sensor;                   // The h/w sensor simulation for the filter.
    CHardwareSimulation *m_HardwareSimulation;  // The h/w isp simulation for this pin, if the pin has been through KSSTATE_ACQUIRE.
    PIN_STATE m_PinState;                       // The state we've put the hardware into.
    PIKSREFERENCECLOCK m_Clock;                 // The clock we've been assigned.
    BOOLEAN m_PendIo;                           // An indication of whether or not we pended I/O for some reason.
    BOOLEAN m_AcquiredResources;                // An indication of whether or not this pin has acquired the necessary hardware resources to operate.
    PKS_VIDEOINFOHEADER m_VideoInfoHeader;
    PKS_BITMAPINFOHEADER m_pBitmapInfoHeader;   // Optional info for image formats.

    //
    // If we are unable to insert all of the mappings in a stream pointer into
    // the "fake" hardware's scatter / gather table, we set this to the
    // stream pointer that's incomplete.  This is done both to make the
    // relasing easier and to make it easier to fake the scatter / gather
    // hardware.
    //
    PKSSTREAM_POINTER m_PreviousStreamPointer;
    LONGLONG m_PresentationTime;
    LONGLONG m_FrameNumber;
    LONGLONG m_DroppedFrames;

    ULONG   m_DesiredFrames;

    //
    // ReleaseAllFrames():
    //
    // Clean up any references we hold on frames in the queue.  This is called
    // when we abruptly stop the fake hardware.
    //
    NTSTATUS
    ReleaseAllFrames ();

    static
    void
    Cleanup (
        _In_    CCapturePin *Pin)
    {
        delete Pin;
    }

    //
    //  Emit metadata here for video or preview pin.
    //
    virtual
    void
    EmitMetadata(
        _Inout_ PKSSTREAM_HEADER   pStreamHeader
    );

    //  Helper function for finding our filter object.
    CCaptureFilter *
    GetFilter()
    {
        return reinterpret_cast <CCaptureFilter *>
               (KsPinGetParentFilter(m_Pin)->Context);
    }

    virtual
    NTSTATUS
    Close(
        _In_ PIRP Irp
    );

    //  Intended to give the pin an opportunity to do something extra with the selected format.
    virtual
    NTSTATUS
    SetFormat(
        _In_opt_    PKSDATAFORMAT OldFormat,
        _In_opt_    PKSMULTIPLE_ITEM OldAttributeList,
        _In_        const KSDATARANGE *DataRange,
        _In_opt_    const KSATTRIBUTE_LIST *AttributeRange
    );

    virtual
    NTSTATUS
    Process( );

    virtual
    NTSTATUS
    SetState(
        _In_    KSSTATE ToState,
        _In_    KSSTATE FromState
    );

    virtual
    void
    Reset();

    virtual
    NTSTATUS
    Connect();  // Pin Connect

    virtual
    void
    Disconnect();   // Pin Disconnect

    virtual
    NTSTATUS 
    CaptureBitmapInfoHeader( );

    virtual
    NTSTATUS 
    Initialize();

    void
    SetDesiredFrames( ULONG n )
    {
        m_DesiredFrames = n;
    }
    
public:
    //
    // CCapturePin():
    //
    // The capture pin's constructor.  Initialize any non-0, non-NULL fields
    // (since new will have zero'ed the memory anyway) and set up our
    // device level pointers for access during capture routines.
    //
    CCapturePin( _In_  PKSPIN Pin);

    //
    // ~CCapturePin():
    //
    // The capture pin's destructor.
    //
    virtual
    ~CCapturePin();

    //
    // ICapturePin::CompleteMapping()
    //
    //  This is the capture pin notification mechanism for mapping completion.
    //  When the device detects that frames have been completed by the fake
    //  hardware, it informs the capture sink through this method.
    //
    virtual
    NTSTATUS
    CompleteMapping(
        _In_ PKSSTREAM_POINTER Clone=nullptr
        );

    //  Update the Pin's Allocator Framing
    virtual
    NTSTATUS
    UpdateAllocatorFraming();

    //  TEST: Move this to a derived CCapturePin class - one 
    //        specifically for testing, not as a sample.
    static
    bool
    GetAcquireFailureKey();

    PIN_STATE   GetState() const
    {
        return m_PinState;
    }

    static
    NTSTATUS
    DispatchClose(
        _In_    PKSPIN Pin,
        _In_    PIRP Irp
    );
    static NTSTATUS DispatchSetState(
        _In_    PKSPIN Pin,
        _In_    KSSTATE ToState,
        _In_    KSSTATE FromState
    );

    static NTSTATUS DispatchSetFormat(
        _In_        PKSPIN Pin,
        _In_opt_    PKSDATAFORMAT OldFormat,
        _In_opt_    PKSMULTIPLE_ITEM OldAttributeList,
        _In_        const KSDATARANGE *DataRange,
        _In_opt_    const KSATTRIBUTE_LIST *AttributeRange
    );

    static NTSTATUS DispatchProcess( _In_ PKSPIN Pin );

    static void DispatchReset( _In_ PKSPIN );
    static NTSTATUS DispatchConnect( _In_ PKSPIN Pin );  // Pin Connect
    static void DispatchDisconnect( _In_ PKSPIN Pin );   // Pin Disconnect

    static
    NTSTATUS
    IntersectHandler(
        _In_        PKSFILTER Filter,
        _In_        PIRP Irp,
        _In_        PKSP_PIN PinInstance,
        _In_        PKSDATARANGE CallerDataRange,
        _In_        PKSDATARANGE DescriptorDataRange,
        _In_        ULONG BufferSize,
        _Out_opt_   PVOID Data OPTIONAL,
        _Out_       PULONG DataSize
    );
};

//
//  Define a dispatch table for a capture pin.  It provides notifications
//  about creation, closure, processing, data formats, etc. using naming 
//  conventions shared by all CCapturePin derived classes.
//
#define DEFINE_CAMERA_KSPIN_DISPATCH( Table, Class )            \
const                                                           \
KSPIN_DISPATCH                                                  \
Table ={                                                        \
    Class::DispatchCreate,          /* Pin Create           */  \
    Class::DispatchClose,           /* Pin Close            */  \
    Class::DispatchProcess,         /* Pin Process          */  \
    Class::DispatchReset,           /* Pin Reset            */  \
    Class::DispatchSetFormat,       /* Pin Set Data Format  */  \
    Class::DispatchSetState,        /* Pin Set Device State */  \
    Class::DispatchConnect,         /* Pin Connect          */  \
    Class::DispatchDisconnect,      /* Pin Disconnect       */  \
    NULL,                           /* Clock Dispatch       */  \
    NULL                            /* Allocator Dispatch   */  \
};

