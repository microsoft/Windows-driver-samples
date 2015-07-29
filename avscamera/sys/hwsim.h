/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        hwsim.h

    Abstract:

        This file contains the definition of the base class, CHardwareSimulation,
        used to simulate a camera's capture stream.

        Our simulation cannot mimic a real camera with any precision.  A real
        camera would likely have dedicated hardware to provide interrupts, do 
        DMA transfers of frames from an internal buffer as well as do scaling 
        and on-the-fly colorspace and planar format conversions so as to supply
        frames for preview, video and photo in varying resolutions and
        colorspaces.  
        
        This simulation does not have such hardware, so instead it synthesizes
        a frame on the fly for each local pin as required.  The frame is
        synthesized from common state information, so the images produced 
        should be comparable from pin to pin.  CHardwareSimulation is used as
        a base class for each simulated pin / hardware.

        A CHardwareSimulation contains state, a list of frame bufferss and a 
        timer that is used in place of a hardware interrupt.  The timer 
        schedules a workitem that is used to synthesize an image, select a 
        buffer and copy that image into the buffer.

        Specializations of CHardwareSimulation provide differing metadata and
        implement specialized functionality for photo sequence and variable
        photo sequence.

    History:

        created 3/9/2001

**************************************************************************/

#define SCATTER_GATHER_MAPPINGS_MAX 128

//  forward class references
class CSensor;
class CHardwareSimulation;
class CHwSimTimer;

//  internal structure definitions

//  Face metadata structure.  Face metadata can be synthesized on any pin.
class CAMERA_METADATA_FACE_DETECTION
    : public CAMERA_METADATA_FACEHEADER
{
public:
    METADATA_FACEDATA           Data[MAX_FACES];
};

//  Information about each frame buffer in the queue.
typedef struct _SCATTER_GATHER_ENTRY
{
    LIST_ENTRY ListEntry;
    PKSSTREAM_POINTER CloneEntry;
    PUCHAR Virtual;
    ULONG ByteCount;
    PHOTOCONFIRMATION_INFO              PhotoConfirmationInfo;
} SCATTER_GATHER_ENTRY, *PSCATTER_GATHER_ENTRY;

//
//  A specialization of a KPassiveTimer that holds a back pointer to the
//  CHardwareSimulation that owns it.
//
class CHwSimTimer
    : public KPassiveTimer
{
    CHardwareSimulation *m_Parent;

public:
    CHwSimTimer(
        _In_    CHardwareSimulation *Parent
    );

    //
    //  Sets the timer
    //
    BOOLEAN
    Set(
        _In_    LARGE_INTEGER        DueTime
    );

private:
    static
    void
    Handler(
        _In_opt_    PVOID           Context
    );
};

class CHardwareSimulation
{
protected:
    CSynthesizer *m_Synthesizer;                // Synthesizer for various formats.
    LONGLONG m_TimePerFrame;
    ULONG m_Width;
    ULONG m_Height;
    ULONG m_ImageSize;
    LONG m_PinID;
    KMutex  m_ListLock;
    LIST_ENTRY m_ScatterGatherMappings;

    NPAGED_LOOKASIDE_LIST m_ScatterGatherLookaside;
    PIN_STATE m_PinState;
    ULONG m_ScatterGatherMappingsMax;
    ULONG m_NumMappingsCompleted;
    ULONG m_ScatterGatherMappingsQueued;
    ULONG m_ScatterGatherBytesQueued;
    ULONG m_NumFramesSkipped;
    ULONG m_InterruptTime;
    LARGE_INTEGER m_StartTime;

    CSensor *m_Sensor;
    CHwSimTimer  m_IsrTimer;

    PHOTOCONFIRMATION_INFO  m_PhotoConfirmationEntry;

    //  Cached values used for Locking simulated state.
    LONGLONG    m_LastReportedExposureTime;
    ULONG       m_LastReportedWhiteBalance;

    //  Used to simulate the skipping of frames when reporting face detection data.
    ULONG       m_FaceDetectionDelay;
    CAMERA_METADATA_FACE_DETECTION  m_LastFaceDetect;

    //  Copy a synthesized framebuffer.
    virtual
    NTSTATUS
    FillScatterGatherBuffers();

    //  Overloadable function for attaching metadata to a frame.
    virtual
    void
    EmitMetadata(
        _Inout_ PKSSTREAM_HEADER   pStreamHeader
    );

    //  Helper function to generate random face metadata.
    void
    EmitFaceMetadata(
        _Inout_ PKSSTREAM_HEADER    pStreamHeader,
        _In_    ULONG               Count,
        _In_    ULONGLONG           Flags,
        _In_    ULONG               DelayLimit=30
    );

    //  Release an entry from our queue.
    void
    FreeSGEntry(
        _In_        PLIST_ENTRY Entry,
        _In_        PCWSTR      EventName=L"StreamPointer freed",
        _In_opt_    LPCGUID     Activity=NULL,
        _In_        ULONG       DataUsed=0
    );

    //  Release our entire queue.
    void
    FreeSGList(
        _In_        PLIST_ENTRY List,
        _In_        PCWSTR      EventName=L"StreamPointer freed",
        _In_opt_    LPCGUID     Activity=NULL
    );

public:

    //  Debug helper
    LONG GetSkippedFrameCount()
    {
        return m_NumFramesSkipped;
    }

    //  Constructor
    CHardwareSimulation (
        _Inout_ CSensor *Sensor,
        _In_    LONG PinID
    );

    //  Destructor
    virtual
    ~CHardwareSimulation ();

    //  Periodic interrupt handling
    virtual
    void
    FakeHardware (
    );

    //  Start streaming.
    virtual
    NTSTATUS
    Start (
        CSynthesizer *ImageSynth,
        IN LONGLONG TimePerFrame,
        IN ULONG Width,
        IN ULONG Height,
        IN ULONG ImageSize
    );

    //  Pause streaming
    virtual
    NTSTATUS
    Pause (
        IN BOOLEAN Pausing
    );

    //  Stop streaming.
    virtual
    NTSTATUS
    Stop();

    //  Reset streaming.
    virtual
    NTSTATUS
    Reset(
        IN PKSPIN Pin
    );

    //  Reset streaming.
    virtual
    NTSTATUS
    Reset();

    //  Add a frame buffer to our queue.
    virtual
    ULONG
    ProgramScatterGatherMappings (
        IN PKSSTREAM_POINTER *Clone,
        IN PUCHAR *Buffer,
        IN PKSMAPPING Mappings,
        IN ULONG MappingsCount,
        IN ULONG MappingStride
    );

    //  Initialization.
    virtual
    BOOLEAN
    Initialize();

    //  Number of frames completed during this run.
    ULONG
    ReadNumberOfMappingsCompleted();

    //  Get the QPC measured in 100ns units.
    ULONGLONG
    ConvertQPCtoTimeStamp(
        _In_opt_    PLARGE_INTEGER pQpcTime
    );

    //  Get the current frame settings.
    virtual
    ISP_FRAME_SETTINGS *
    GetIspSettings(void);

    //  So a photo sim can ask a preview sim for a confirmation frame.
    NTSTATUS
    GeneratePhotoConfirmation(
        _In_ ULONG      PfsFrameNumber,
        _In_ LONGLONG   time
    );

    //  Get the exposure value for the current frame.
    LONGLONG
    GetCurrentExposureTime();

    //  Get the White balance for the current frame.
    ULONG
    GetCurrentWhiteBalance();

    //  Get the ISO Speed value for the current frame.
    ULONG
    GetCurrentISOSpeed();

    //  Get the Lens position for the current frame.
    ULONG
    GetCurrentLensPosition();

    //  Get the Scene Mode for the current frame.
    ULONG
    GetCurrentSceneMode();

    //  Translate Exposure Mode to an Exif Exposure Program value
    USHORT
    GetExposureProgram();

    //  Pin state accessor...
    PIN_STATE
    GetState()
    {
        return m_PinState;
    }

    friend class CHwSimTimer;
};

//
//  Sets the timer using our callback and our parent CHardwareSimulation.
//
inline
BOOLEAN
CHwSimTimer::
Set(
    _In_    LARGE_INTEGER        DueTime
)
{
    return KPassiveTimer::Set( DueTime, Handler, m_Parent );
}
