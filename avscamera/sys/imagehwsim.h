/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        ImageHwSim.h

    Abstract:

        This file contains the definition of the CImageHardwareSimulation class.

        This is a specialization of CHardwareSimulation that provides photo-
        specific metadata and implements specialized functionality for photo, 
        photo sequence and variable photo sequence.

    History:

        created 3/9/2001

**************************************************************************/

//  forward references
struct ISP_FRAME_SETTINGS;

class CImageHardwareSimulation
    : public CHardwareSimulation
{
private:
    LIST_ENTRY      m_BurstList;
    PWSTR           m_szwFramePath;
    BOOLEAN         m_bTriggered;
    BOOLEAN         m_bEndOfSequence;
    BOOLEAN         m_bFlashed;
    BOOLEAN         m_bPastBufferTrigger;
    PIN_MODE        m_PinMode;
    ULONG           m_PastBufferCount;
    ULONGLONG       m_TriggerTime;
    PIKSREFERENCECLOCK m_Clock;
    ULONGLONG       m_FlashStatus;

    //  Settings for VPS simulation.
    ULONG               m_PfsLoopLimit;
    ULONG               m_PfsFrameLimit;
    ISP_FRAME_SETTINGS *m_pIspSettings;          //
    ULONG               m_PfsLoopNumber;           // Number of loops we've done.
    ULONG               m_PfsFrameNumber;
    ULONGLONG           m_GlobalFrameNumber;    //  Informational only.

    //  Copy a synthesized framebuffer.
    virtual
    NTSTATUS
    FillScatterGatherBuffers();

    //  Find and complete any history frames.
    NTSTATUS
    CompletePastBuffers();

    //  Put an item onto the history list.
    void
    PushCloneList(_Inout_  PSCATTER_GATHER_ENTRY SGEntry);

    //  Complete the history list.
    NTSTATUS
    CompleteCloneList();

    //  Add Photo-specific metadata.
    void
    EmitMetadata(
        _Inout_ PKSSTREAM_HEADER   pStreamHeader
    );

    //  Does the current frame need a photo confirmation?
    BOOLEAN
    IsPhotoConfirmationNeeded();

    //  Helper function that gets a buffer full of image metadata.
    METADATA_IMAGEAGGREGATION
    GetMetadata();

public:
    PKSSTREAM_POINTER m_pClone;
    PHOTOCONFIRMATION_INFO  m_PhotoConfirmationInfo;

    //  Constructor
    CImageHardwareSimulation (
        _Inout_ CSensor *Sensor,
        _In_    LONG    PinID
    );

    //  Destructor
    virtual
    ~CImageHardwareSimulation();

    //  Periodic interrupt handling
    virtual
    void
    FakeHardware();

    //  Start streaming.
    virtual
    NTSTATUS
    Start(
        _In_    CSynthesizer *ImageSynth,
        _In_    ULONG Width,
        _In_    ULONG Height,
        _In_    ULONG ImageSize,
        _In_    PIN_MODE pinMode
    );

    //  Stop streaming.
    virtual
    NTSTATUS
    Stop();

    virtual
    NTSTATUS
    Reset();

    //  Take a picture or start a photo sequence.
    virtual
    NTSTATUS
    Trigger(
        _In_    LONG mode
    );

    //  Limit the rate of frames produced.
    virtual
    NTSTATUS
    SetPhotoFrameRate (
        _In_    ULONGLONG TimePerFrame
    );

    //  Get the rate of frames produced.
    virtual
    ULONGLONG
    GetPhotoFrameRate()
    {
        return m_TimePerFrame;
    }

    //  Set the master clock.
    NTSTATUS
    SetClock(
        _In_    PKSPIN pin
    );

    //  Identify exactly when the user pressed that button.
    void
    SetTriggerTime(
        _In_    ULONGLONG TriggerTime
    );

    //  Report the current trigger time.
    ULONGLONG
    GetTriggerTime()
    {
        return m_TriggerTime;
    }

    //  Normal vs. Photo sequence + number of history frames.
    NTSTATUS
    SetMode(
        _In_    ULONGLONG Flags,
        _In_    ULONG PastBuffers
    );

    NTSTATUS
    SetFlashStatus(
        _In_    ULONGLONG ulFlashStatus
    );

    //  Program the Per Frame Settings for simulation
    //  We do it before calling start so we can be ready
    //  to program our simulation's hardware.
    //  Note: We make a local copy.
    NTSTATUS
    SetPFS(
        _In_    ISP_FRAME_SETTINGS  *pIspSettings,
        _In_    ULONG               FrameLimit,
        _In_    ULONG               LoopLimit
    );

    //  Advance our frame and loop pointers to the next PFS settings.
    bool
    AdvanceFrameCounter(void);

    //  Determine if we're at the EOS.
    bool
    IsPfsEOS(void);

    //  Determine if we're in a Variable Photo Sequence.
    BOOL
    IsPfsActive(void);

    //  Get the current frame settings.
    virtual
    ISP_FRAME_SETTINGS *
    GetIspSettings(void);
};
