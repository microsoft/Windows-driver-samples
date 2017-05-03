/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    WaveReader.h

Abstract:

    Declaration of SYSVAD wave reader.


--*/
#ifndef _SYSVAD_WAVEREADER_H
#define _SYSVAD_WAVEREADER_H

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits.h>

#define NUM_OF_CHUNK_FOR_FILE_READ 2

class CWaveReader;

// Wave header structure decleration 
typedef CWaveReader *pWaveReader;
typedef struct _WAVEHEADER
{
    BYTE chunkId[4];
    ULONG chunkSize;
    BYTE format[4];
    BYTE subChunkId[4];
    ULONG subChunkSize;
    WORD audioFormat;
    WORD numChannels;
    ULONG sampleRate;
    ULONG bytesPerSecond;
    WORD blockAlign;
    WORD bitsPerSample;
    BYTE dataChunkId[4];
    ULONG dataSize;
}WAVEHEADER;

typedef struct _CHUNKDESCRIPTOR
{
    PBYTE pStartAddress;        // Starting address of the chunk
    ULONG ulChunkLength;        // Length of the chunk
    bool  bIsChunkEmpty;        // If the chunk is empty
    WORD  chunkNumber;          // it will be 0, 1, 2 and so on for different chunks
}CHUNKDESCRIPTOR;
typedef CHUNKDESCRIPTOR *PCHUNKDESCRIPTOR;

/*
    The idea here is to allocate one second long worth of buffer and divide it into NUM_OF_CHUNK_FOR_FILE_READ chunks.
    In one file read operation we read and fill one chunk data . The chunk will be emptied every 10 ms by OS.
    Once the OS empties one chunk data we schedule a workitem to read and fill next available chunk. 
*/

typedef struct _WAVEDATAQUEUE
{
    PBYTE pWavData;     // Pointer to the temporary buffer for reading one second worth of data from wave file
    ULONG ulLength;     // length of pWavData in bytes
    ULONG ulReadPtr;    // current reading position in pWavData in bytes
    bool  bEofReached;  // This will be set once the eof is reached.
    bool  firstChunkIsRead; // This will be true after first chunk is read
    WORD  currentExecutedChunk;
    CHUNKDESCRIPTOR sChunkDescriptor[NUM_OF_CHUNK_FOR_FILE_READ];
}WAVEDATAQUEUE;
typedef WAVEDATAQUEUE *PWAVEDATAQUEUE;

// Parameter to workitem.
#include <pshpack1.h>
typedef struct _READWORKER_PARAM {
    PIO_WORKITEM     WorkItem;      // Pointer to the workitem
    KEVENT           EventDone;     // Used for synchronizing a workitem for scheduling. 
    pWaveReader      PtrWaveReader;   // pointer to the wavereader class.
    PCHUNKDESCRIPTOR PtrChunkDescriptor;   // chunk descriptor for the chunk, which needs to be filled after file read
} READWORKER_PARAM;
typedef READWORKER_PARAM *PREADWORKER_PARAM;
#include <poppack.h>

IO_WORKITEM_ROUTINE ReadFrameWorkerCallback;

// Wave Reader class

class CWaveReader
{

public:
    HANDLE                      m_FileHandle;       // Wave File handle.
    WORD                        m_ChannelCount;     // Number of Channels for the stream during stream init
    WORD                        m_BitsPerSample;    // Number of Bits per sample for the stream during stream init
    DWORD                       m_SamplesPerSecond; // Number of Sample per second for the stream during stream init
    bool                        m_Mute;             // Capture Zero buffer if mute
    OBJECT_ATTRIBUTES           m_objectAttributes; // Used for opening file.
    WAVEDATAQUEUE               m_WaveDataQueue;    // Big buffer data object and its current state
    KMUTEX                      m_FileSync;         // Synchronizes file access
    WAVEHEADER                  m_WaveHeader;
    ULONG                       m_LoopCount;        // Number of times to loop the wave file

public:
    CWaveReader();
    ~CWaveReader();

    NTSTATUS Init
    (
        _In_ PWAVEFORMATEXTENSIBLE WfExt,
        _In_ PUNICODE_STRING puiFileName,
        _In_ ULONG loopCount
    );

    VOID ReadWaveData
    (
        _Out_writes_bytes_(BufferLength) BYTE  *Buffer,
        _In_ ULONG      BufferLength
    );

    VOID SetMute(_In_ bool Value)
    {
        m_Mute = Value;
    }

    void WaitAllWorkItems();

    // Static allocation totally related to the workitems for reading data from wavefile and putting it to chunk buffer
    static PDEVICE_OBJECT       m_pDeviceObject;
    static PREADWORKER_PARAM    m_pWorkItems;
    static NTSTATUS  InitializeWorkItems(_In_ PDEVICE_OBJECT  DeviceObject);
    static PREADWORKER_PARAM GetNewWorkItem();
    static VOID DestroyWorkItems();

private:
    VOID ReadWavChunk(PCHUNKDESCRIPTOR PtrChunkDescriptor);

    bool IsAllChunkEmpty();

    NTSTATUS OpenWaveFile(PUNICODE_STRING puiFileName);

    NTSTATUS FileClose();

    NTSTATUS FileReadHeader();

    NTSTATUS FileOpen();

    VOID CopyDataFromRingBuffer
    (
        _Out_writes_bytes_(BufferLength) BYTE       *Buffer,
        _In_                             ULONG      BufferLength
    );

    NTSTATUS AllocateBigBuffer();
    NTSTATUS ReadHeaderAndFillBuffer();
    friend IO_WORKITEM_ROUTINE ReadFrameWorkerCallback;
};

#endif // _SYSVAD_WAVEREADER_H

