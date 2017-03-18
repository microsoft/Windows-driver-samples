/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        wave.h

    Abstract:

        Wave object header.

    History:

        Created 6/28/01

**************************************************************************/

//
// The CWaveObject is a class which will parse PCM wave files, read the
// data, and expose the data in a loop.  This allows the sample to "synthesize"
// audio data by using any PCM wave file the user wishes.
//
class CWaveObject {

private:

    //
    // The wave format.
    //
    WAVEFORMATEX m_WaveFormat;

    //
    // The wave data.
    //
    PUCHAR m_WaveData;

    //
    // The size of the wave data.
    //
    ULONG m_WaveSize;

    //
    // The filename for the wave file.  This string must be constant and 
    // static over the lifetime of the wave object.
    //
    PWCHAR m_FileName;

    //
    // The time we have synthesized to.
    //
    LONGLONG m_SynthesisTime;

    //
    // The pointer into the wave data that we have synthesized to.
    //
    ULONG m_WavePointer;

    //
    // ParseBlock():
    //
    // Parse the wave file, starting at the specified location, until the
    // specified block has been found.  The pointer will be updated to
    // point to the block data and the amount of data in the block will
    // be returned in a variable. 
    //
    NTSTATUS
    ParseForBlock (
        IN HANDLE FileHandle,
        IN ULONG BlockHeader,
        IN OUT PLARGE_INTEGER BlockPointer,
        OUT PULONG BlockSize
        );

public:

    //
    // CWaveObject():
    //
    // Construct a new wave object using the specified file name.
    //
    CWaveObject (
        _In_ LPWSTR FileName
        ) :
        m_FileName (FileName)
    {
        m_WaveData = NULL;
    }

    //
    // ~CWaveObject():
    //
    // Destroy a wave object.
    //
    ~CWaveObject (
        );

    //
    // ParseAndRead():
    //
    // Parse the wave file and read it into an internally allocated buffer
    // inside the wave object.  This is preparation to synthesize looped
    // audio based on the wave.
    //
    NTSTATUS
    ParseAndRead (
        );

    //
    // WriteRange():
    //
    // Given the address of a KSDATARANGE_AUDIO, write out a range which
    // matches exactly the specifications of the wave we're using to
    // synthesize audio data.
    //
    // The GUIDs must be filled out already.  This only fills out the 
    // channel, bps, and freq fields.
    //
    void
    WriteRange (
        PKSDATARANGE_AUDIO AudioRange
        );

    //
    // SynthesizeTo():
    //
    // Given a specific stream time, synthesize from the current stream time
    // (assume 0) to the supplied stream time.
    //
    ULONG
    SynthesizeTo (
        IN LONGLONG StreamTime,
        IN PVOID Data,
        IN ULONG BufferSize
        );

    //
    // SynthesizeFixed():
    //
    // Given a specific amount of time, synthesize forward in time that
    // particular amount.  Units expressed in 100nS increments.
    //
    ULONG
    SynthesizeFixed (
        IN LONGLONG TimeDelta,
        IN PVOID Data,
        IN ULONG BufferSize
        );

    //
    // SkipFixed():
    //
    // Given a specific amount of time, skip forward in time that
    // particular amount.  Units expressed in 100nS increments.
    //
    void
    SkipFixed (
        IN LONGLONG TimeDelta
        );

    //
    // Reset():
    //
    // Reset the synthesis time and block pointers.  This will cause the
    // clock with respect to this wave object to go to zero.
    //
    void
    Reset (
        )
    {
        m_WavePointer = 0;
        m_SynthesisTime = 0;
    }

    //
    // Cleanup():
    //
    // This is a bag cleanup callback.  It merely deletes the wave object
    // instead of letting the default of ExFreePool free it.
    //
    static
    void
    Cleanup (
        IN CWaveObject *This
        )
    {
        delete This;
    }

};
