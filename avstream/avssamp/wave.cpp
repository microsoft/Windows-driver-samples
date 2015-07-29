/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        wave.cpp

    Abstract:

        Wave object implementation.

    History:

        Created 6/28/01

**************************************************************************/

#include "avssamp.h"

/**************************************************************************

    PAGED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CWaveObject::
~CWaveObject (
    )

/*++

Routine Description:

    Destroy a wave object.

Arguments:

    None

Return Value:

    None

--*/

{
    PAGED_CODE();

    if (m_WaveData) {
        ExFreePool (m_WaveData);
    }

}

/*************************************************/


NTSTATUS
CWaveObject::
ParseForBlock (
    IN HANDLE FileHandle,
    IN ULONG BlockHeader,
    IN OUT PLARGE_INTEGER BlockPosition,
    OUT PULONG BlockSize
    )

/*++

Routine Description:

    Given that BlockPosition points to the offset of the start of a RIFF block,
    continue parsing the specified file until a block with the header of
    BlockHeader is found.  Return the position of the block data and the size
    of the block.

Arguments:

    FileHandle -
        Handle to the file to parse

    BlockHeader -
        The block header to scan for

    BlockPosition -
        INPUT : Points to the block header to start at
        OUTPUT: If successful, points to the block data for the sought block
                If unsuccessful, unchanged

    BlockSize -
        On output, if successful -- the size of the sought block will be 
        placed here

Return Value:

    Success / Failure of the search

--*/

{

    PAGED_CODE();

    NTSTATUS Status;
    ULONG FmtBlockSize = 0;
    LARGE_INTEGER ReadPos = *BlockPosition;
    IO_STATUS_BLOCK iosb;

    while (1) {
        ULONG BlockHeaderData [2];

        Status = ZwReadFile (
            FileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            BlockHeaderData,
            sizeof (BlockHeaderData),
            &ReadPos,
            NULL
            );

        if (NT_SUCCESS (Status)) {
            if (BlockHeaderData [0] == BlockHeader) {
                FmtBlockSize = BlockHeaderData [1];
                ReadPos.QuadPart += 0x8;
                break;
            } else {
                //
                // This isn't a format block.  Just ignore it.  All we
                // care about is the format block and the PCM data.
                //
                ReadPos.QuadPart += BlockHeaderData [1] + 0x8;
            }
        } else {
            break;
        }

    }

    if (FmtBlockSize == 0) {
        Status = STATUS_NOT_FOUND;
    } else {
        *BlockPosition = ReadPos;
        *BlockSize = FmtBlockSize;
    }

    return Status;

}

/*************************************************/


NTSTATUS
CWaveObject::
ParseAndRead (
    )

/*++

Routine Description:

    Parse the wave file and read the data into an internally allocated
    buffer.  This prepares to synthesize audio data from the wave
    object.

Arguments:

    None

Return Value:

    Success / Failure

        If the wave is unrecognized, unparsable, or insufficient memory
        exists to allocate the internal buffer, an error code will
        be returned and the object will be incapable of synthesizing
        audio data based on the wave.

--*/

{

    PAGED_CODE();

    IO_STATUS_BLOCK iosb;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE FileHandle = NULL;
    FILE_OBJECT *FileObj;

    RtlInitUnicodeString (&FileName, m_FileName);

    InitializeObjectAttributes (
        &ObjectAttributes,
        &FileName,
        (OBJ_CASE_INSENSITIVE |
        OBJ_KERNEL_HANDLE),
        NULL,
        NULL
        );

    Status = ZwCreateFile (
        &FileHandle,
        GENERIC_READ | SYNCHRONIZE,
        &ObjectAttributes,
        &iosb,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (NT_SUCCESS (Status)) {
        ULONG RiffWaveHeader [3];

        //
        // Read the header: RIFF size WAVE
        //
        Status = ZwReadFile (
            FileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            RiffWaveHeader,
            sizeof (RiffWaveHeader),
            NULL,
            NULL
            );

        //
        // Ensure that this is a RIFF file and it's a WAVE.
        //
        if (NT_SUCCESS (Status)) {

            if (RiffWaveHeader [0] != 'FFIR' ||
                RiffWaveHeader [2] != 'EVAW') {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    //
    // Find the wave format block and ensure it's WAVEFORMATEX and PCM
    // data.  Otherwise, this can't parse the wave.
    //
    LARGE_INTEGER ReadPos;
    ReadPos.QuadPart = 0xc;
    ULONG FmtBlockSize = 0;

    if (NT_SUCCESS (Status)) {
        Status = ParseForBlock (FileHandle, ' tmf', &ReadPos, &FmtBlockSize);
    }

    //
    // If the format block was not found, the file cannot be parsed.  If the
    // format block is unrecognized, the file cannot be parsed.
    //
    if (FmtBlockSize >= sizeof (m_WaveFormat)) {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS (Status)) {
        Status = ZwReadFile (
            FileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            &m_WaveFormat,
            FmtBlockSize,
            &ReadPos,
            NULL
            );
    }

    if (NT_SUCCESS (Status)) {
        if (m_WaveFormat.wFormatTag != WAVE_FORMAT_PCM) {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    ReadPos.QuadPart += FmtBlockSize;

    //
    // Find the data block and read it in.
    //
    ULONG DataBlockSize;
    if (NT_SUCCESS (Status)) {
        Status = ParseForBlock (FileHandle, 'atad', &ReadPos, &DataBlockSize);
    }

    //
    // Perform a slight validation.
    //
    if (NT_SUCCESS (Status) && 
        (DataBlockSize == 0 || 
        (DataBlockSize & (m_WaveFormat.nBlockAlign - 1)))) {

        Status = STATUS_INVALID_PARAMETER;
    }

    //
    // If we're okay so far, allocate memory for the wave data.
    //
    if (NT_SUCCESS (Status)) {
        m_WaveData = reinterpret_cast <PUCHAR> (
            ExAllocatePoolWithTag (NonPagedPool, DataBlockSize, AVSSMP_POOLTAG)
            );

        if (!m_WaveData) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Read the wave data in.
    //
    if (NT_SUCCESS (Status)) {
        Status = ZwReadFile (
            FileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            m_WaveData,
            DataBlockSize,
            &ReadPos,
            NULL
            );

        m_WaveSize = DataBlockSize;
    }

    //
    // If we failed, clean up.
    //
    if (!NT_SUCCESS (Status)) {
        if (m_WaveData) {
            ExFreePool (m_WaveData);
            m_WaveData = NULL;
        }
    }

    if (FileHandle) {
        ZwClose (FileHandle);
    }

    return Status;
}

/*************************************************/


void
CWaveObject::
WriteRange (
    OUT PKSDATARANGE_AUDIO DataRange
    )

/*++

Routine Description:

    Fill out the extended portion of the audio data range at DataRange.  This
    includes the channel, bps, and frequency fields.

Arguments:

    DataRange -
        The data range to fill out

Return Value:

    None

--*/

{

    PAGED_CODE();

    DataRange -> MaximumChannels = m_WaveFormat.nChannels;
    DataRange -> MinimumBitsPerSample =
        DataRange -> MaximumBitsPerSample = 
        m_WaveFormat.wBitsPerSample;
    DataRange -> MinimumSampleFrequency =
        DataRange -> MaximumSampleFrequency =
        m_WaveFormat.nSamplesPerSec;


}

/**************************************************************************

    LOCKED CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


void
CWaveObject::
SkipFixed (
    IN LONGLONG TimeDelta
    )

/*++

Routine Description:

    Skip ahead a specific time delta within the wave.

Arguments:
    
    TimeDelta -
        The amount of time to skip ahead.

--*/

{
    if (TimeDelta > 0)  {

        //
        // Compute the number of bytes of audio data necessary to move the 
        // stream forward TimeDelta time.  Remember that TimeDelta is in 
        // units of 100nS.
        //
        ULONG Samples = (ULONG)(
            (m_WaveFormat.nSamplesPerSec * TimeDelta) / 10000000
            );
    
        ULONG Bytes = Samples * (m_WaveFormat.wBitsPerSample / 8) *
            m_WaveFormat.nChannels;
    
        m_WavePointer = (m_WavePointer + Bytes) % m_WaveSize;
    
        m_SynthesisTime += TimeDelta;

    }

}


ULONG
CWaveObject::
SynthesizeFixed (
    IN LONGLONG TimeDelta,
    IN PVOID Buffer,
    IN ULONG BufferSize
    )

/*++

Routine Description:

    Copy wave data from our wave block in order to synthesize forward in time
    TimeDelta (in 100nS units).

Arguments:

    TimeDelta -
        The amount of time to move the stream (in 100nS increments)

    Buffer -
        The buffer to synthesize into

    BufferSize -
        The size of the buffer

Return Value:

    Number of bytes synthesized.

--*/

{

    //
    // If there is no time delta, return 0.
    //
    if (TimeDelta < 0) 
        return 0;

    //
    // Compute the number of bytes of audio data necessary to move the stream
    // forward TimeDelta time.  Remember that TimeDelta is in units of 100nS.
    //
    ULONG Samples = (ULONG)(
        (m_WaveFormat.nSamplesPerSec * TimeDelta) / 10000000
        );

    ULONG Bytes = Samples * (m_WaveFormat.wBitsPerSample / 8) *
        m_WaveFormat.nChannels;

    //
    // Now that we have a specified number of bytes, we determine how many
    // to really copy based on the Size of the buffer.
    //
    if (Bytes > BufferSize) Bytes = BufferSize;

    //
    // Because the buffer is looping, this may multiple distinct copies.  For
    // large wave files, this may be two chunks.  For small wave files, this
    // may be MANY distinct chunks.
    //
    ULONG BytesRemaining = Bytes;
    PUCHAR DataCopy = reinterpret_cast <PUCHAR> (Buffer);

    while (BytesRemaining) {
        ULONG ChunkCount = m_WaveSize - m_WavePointer;
        if (ChunkCount > BytesRemaining) ChunkCount = BytesRemaining;

        RtlCopyMemory (
            DataCopy,
            m_WaveData + m_WavePointer,
            ChunkCount
            );

        m_WavePointer += ChunkCount;
        if (m_WavePointer >= m_WaveSize) m_WavePointer -= m_WaveSize;

        BytesRemaining -= ChunkCount;
        DataCopy += ChunkCount;

    }

    //
    // Consider that we have synthesized up to the specified time.  If the
    // buffer was not large enough to do this, we'll end up falling behind
    // the synthesis time.  This does not skip samples.
    //
    m_SynthesisTime += TimeDelta;

    return Bytes;

}


ULONG
CWaveObject::
SynthesizeTo (
    IN LONGLONG StreamTime,
    IN PVOID Buffer,
    IN ULONG BufferSize
    )

/*++

Routine Description:

    Copy wave data from our wave block in order to synthesize the stream
    up to the specified stream time.  If the buffers are not large enough,
    this will fall behind on synthesis.

Arguments:

    StreamTime -
        The time to synthesize up to

    Buffer -
        The buffer to copy synthesized wave data into

    BufferSize -
        The size of the buffer

Return Value:

    The number of bytes used.

--*/

{

    LONGLONG TimeDelta = StreamTime - m_SynthesisTime;

    return SynthesizeFixed (TimeDelta, Buffer, BufferSize);


}

