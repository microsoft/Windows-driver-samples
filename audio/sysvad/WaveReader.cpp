/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:
    WaveReader.cpp

Abstract:
    Implementation of SYSVAD wave file reader

    To read data from disk, this class maintains a circular data buffer. This buffer is segmented into multiple chunks of 
    big buffer (Though we only need two, so it is set to two now). Initially we fill first two chunks and once a chunk gets emptied 
    by OS, we schedule a workitem to fill the next available chunk.


--*/

#pragma warning (disable : 4127)
#pragma warning (disable : 26165)

#include <sysvad.h>
#include "WaveReader.h"
#include "wdm.h"
#include <ntdef.h>

#define FILE_NAME_BUFFER_TAG        'WRT1'
#define WAVE_DATA_BUFFER_TAG        'WRT2'
#define WORK_ITEM_BUFFER_TAG        'WRT3'

#define MAX_READ_WORKER_ITEM_COUNT       15

#pragma code_seg("PAGE")

/*++

Routine Description:
    Ctor: basic init.

--*/

CWaveReader::CWaveReader()
: m_ChannelCount(0),
  m_BitsPerSample(0),
  m_SamplesPerSecond(0),
  m_LoopCount(1),
  m_Mute(false),
  m_FileHandle(NULL)
{
    PAGED_CODE();
    m_WaveDataQueue.pWavData = NULL;
    KeInitializeMutex(&m_FileSync, 0);
}

/*++

Routine Description:
    Dtor: free resources.

--*/
CWaveReader::~CWaveReader()
{
    PAGED_CODE();
    if (STATUS_SUCCESS == KeWaitForSingleObject
    (
        &m_FileSync,
        Executive,
        KernelMode,
        FALSE,
        NULL
    ))
    {
        if (m_WaveDataQueue.pWavData != NULL)
        {
            ExFreePoolWithTag(m_WaveDataQueue.pWavData, WAVE_DATA_BUFFER_TAG);
            m_WaveDataQueue.pWavData = NULL;
        }

        FileClose();
        KeReleaseMutex(&m_FileSync, FALSE);
    }
    
}

/*++

Routine Description:
    - Initializing the workitems. These workitems will be scheduled asynchronously by the OS. 
    - When these work items will be scheduled the wave file will be read and the data 
    - will be put inside the big chunks.

Arguments:
    Device object

Return Value:
    NT status code.

--*/

NTSTATUS CWaveReader::InitializeWorkItems(_In_ PDEVICE_OBJECT DeviceObject)
{
    PAGED_CODE();

    ASSERT(DeviceObject);

    NTSTATUS                    ntStatus = STATUS_SUCCESS;

    DPF_ENTER(("[CWaveReader::InitializeWorkItems]"));

    if (m_pWorkItems != NULL)
    {
        return ntStatus;
    }

    m_pWorkItems = (PREADWORKER_PARAM)
        ExAllocatePoolWithTag
        (
            NonPagedPoolNx,
            sizeof(READWORKER_PARAM) * MAX_READ_WORKER_ITEM_COUNT,
            'RDPT'
        );
    if (m_pWorkItems)
    {
        for (int i = 0; i < MAX_READ_WORKER_ITEM_COUNT; i++)
        {

            m_pWorkItems[i].WorkItem = IoAllocateWorkItem(DeviceObject);
            if (m_pWorkItems[i].WorkItem == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            KeInitializeEvent
            (
                &m_pWorkItems[i].EventDone,
                NotificationEvent,
                TRUE
            );
        }
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
} 

/*++

Routine Description:
- Wait for all the scheduled workitems to finish.

--*/


//=============================================================================
void CWaveReader::WaitAllWorkItems()
{
    PAGED_CODE();

    for (int i = 0; i < MAX_READ_WORKER_ITEM_COUNT; i++)
    {
        KeWaitForSingleObject
        (
            &(m_pWorkItems[i].EventDone),
            Executive,
            KernelMode,
            FALSE,
            NULL
        );
    }
} 

/*++

Routine Description:
    - Deallocating the workitems. 

--*/


VOID CWaveReader::DestroyWorkItems()
{
    PAGED_CODE();

    if (m_pWorkItems)
    {
        for (int i = 0; i < MAX_READ_WORKER_ITEM_COUNT; i++)
        {
            if (m_pWorkItems[i].WorkItem != NULL)
            {
                IoFreeWorkItem(m_pWorkItems[i].WorkItem);
                m_pWorkItems[i].WorkItem = NULL;
            }
        }
        ExFreePoolWithTag(m_pWorkItems, WORK_ITEM_BUFFER_TAG);
        m_pWorkItems = NULL;
    }
} 

/*++

Routine Description:
    - Get a free work item to schedule a file read operation.

--*/
#pragma code_seg()
PREADWORKER_PARAM CWaveReader::GetNewWorkItem()
{
    LARGE_INTEGER               timeOut = { 0 };
    NTSTATUS                    ntStatus;

    for (int i = 0; i < MAX_READ_WORKER_ITEM_COUNT; i++)
    {
        ntStatus =
            KeWaitForSingleObject
            (
                &m_pWorkItems[i].EventDone,
                Executive,
                KernelMode,
                FALSE,
                &timeOut
            );
        if (STATUS_SUCCESS == ntStatus)
        {
            if (m_pWorkItems[i].WorkItem)
                return &(m_pWorkItems[i]);
            else
                return NULL;
        }
    }

    return NULL;
} 

/*++
Routine Description:
- This routine will enqueue a workitem for reading wave file and putting 
- the data into the chunk buffer.

Arguments:
    Chunk descriptor for the chunk to be filled.
--*/

VOID CWaveReader::ReadWavChunk(PCHUNKDESCRIPTOR pChunkDescriptor)
{
    PREADWORKER_PARAM           pParam = NULL;

    DPF_ENTER(("[CWaveReader::ReadWavChunk]"));

    pParam = GetNewWorkItem();
    if (pParam)
    {
        pParam->PtrWaveReader = this;
        pParam->PtrChunkDescriptor = pChunkDescriptor;
        KeResetEvent(&pParam->EventDone);
        IoQueueWorkItem(pParam->WorkItem, ReadFrameWorkerCallback,
            DelayedWorkQueue, (PVOID)pParam);
    }
} 

#pragma code_seg("PAGE")

IO_WORKITEM_ROUTINE ReadFrameWorkerCallback;
/*
Routine Description:
- This routine will be called by the OS. It will fill the chunk buffer, defined by the chunk descriptor
- If end of file is reached it will mark the end of file as true.

Arguments:
    pDeviceObject - Device object
    Context       - pointer to reader worker params
*/

VOID ReadFrameWorkerCallback
(
    _In_        PDEVICE_OBJECT         pDeviceObject,
    _In_opt_    PVOID                  Context
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(pDeviceObject);
    pWaveReader                  pWavRd;
    PREADWORKER_PARAM           pParam = (PREADWORKER_PARAM)Context;

    if (NULL == pParam)
    {
        // This is completely unexpected, assert here.
        //
        ASSERT(pParam);
        goto exit;
    }

    pWavRd = pParam->PtrWaveReader;

    if (pWavRd == NULL)
    {
        goto exit;
    }
    if (STATUS_SUCCESS == KeWaitForSingleObject
    (
        &pWavRd->m_FileSync,
        Executive,
        KernelMode,
        FALSE,
        NULL
    )) 
    {

        NTSTATUS                    ntStatus = STATUS_SUCCESS;

        ASSERT(Context);
        
        IO_STATUS_BLOCK             ioStatusBlock;

        if (pParam->WorkItem)
        {
            if (pWavRd->m_WaveDataQueue.bEofReached || pWavRd->m_WaveDataQueue.pWavData == NULL)
            {
                KeReleaseMutex(&pWavRd->m_FileSync, FALSE);
                goto exit;
            }

            if (pParam->PtrChunkDescriptor->pStartAddress != NULL)
            {
                ntStatus = ZwReadFile(pWavRd->m_FileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    pParam->PtrChunkDescriptor->pStartAddress,
                    pParam->PtrChunkDescriptor->ulChunkLength,
                    NULL,
                    NULL);

                // "initial worktiem scheduling issue in this file"
                // If this is the first read for the file, set the read pointer such that it is equal to the chunks first byte.
                if (!pWavRd->m_WaveDataQueue.firstChunkIsRead)
                {
                    pWavRd->m_WaveDataQueue.firstChunkIsRead = true;
                    pWavRd->m_WaveDataQueue.ulReadPtr = pParam->PtrChunkDescriptor->chunkNumber * pParam->PtrChunkDescriptor->ulChunkLength;
                }

                if (ioStatusBlock.Information != pParam->PtrChunkDescriptor->ulChunkLength)
                {
                    pWavRd->m_LoopCount--;
                    if (pWavRd->m_LoopCount > 0)
                    {
                        // The assumption here is that the wave file will be a perfect wave file with no partial frames/bytes at the end.
                        // If we need to support files which are not aligned at the end, we need to make modification in this code.
                        // Currently this will be used only for internal testing. In this we will have controlled wave files.
                        ULONG numBytesRead = (ULONG)ioStatusBlock.Information;

                        LARGE_INTEGER startingOffset;
                        startingOffset.QuadPart = sizeof(WAVEHEADER);

                        ntStatus = ZwReadFile(pWavRd->m_FileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &ioStatusBlock,
                            pParam->PtrChunkDescriptor->pStartAddress + numBytesRead,
                            (ULONG)pParam->PtrChunkDescriptor->ulChunkLength - numBytesRead,
                            &startingOffset,
                            NULL);
                    }
                    else
                    {
                        pWavRd->m_WaveDataQueue.bEofReached = true;
                    }
                }

                pParam->PtrChunkDescriptor->bIsChunkEmpty = false;
            }
        }

        KeReleaseMutex(&pWavRd->m_FileSync, FALSE);
    }

exit:
    KeSetEvent(&pParam->EventDone, 0, FALSE);
}

/*++

Routine Description:
-   If all the chunks are empty this resturn true.

--*/

#pragma code_seg()
bool CWaveReader::IsAllChunkEmpty()
{
    for (int i = 0; i < NUM_OF_CHUNK_FOR_FILE_READ; i++)
    {
        if (!m_WaveDataQueue.sChunkDescriptor[i].bIsChunkEmpty)
        {
            return false;
        }
    }
    return true;
}

/*++
Routine Description:
    - This routine does the actual copy of data from the chunk buffer to the buffer provided by OS.
    - If it empties the current chunk buffer, then it sets it state to empty and then enqueue a workitem 
    - to read data from the wave file and put it to the next available chunk buffer. 

Arguments:
    Buffer - Pointer to the OS buffer
    BufferLength - Length of the data to be filled (in bytes)

--*/
#pragma code_seg()
VOID CWaveReader::CopyDataFromRingBuffer
(
    _Out_writes_bytes_(BufferLength) BYTE       *Buffer,
    _In_                             ULONG      BufferLength
)
{
    if (IsAllChunkEmpty() && !m_WaveDataQueue.firstChunkIsRead)
    {
        RtlZeroMemory(Buffer, BufferLength);
    }
    else
    {
        ULONG prevChunk = (m_WaveDataQueue.ulReadPtr*NUM_OF_CHUNK_FOR_FILE_READ )/ m_WaveDataQueue.ulLength;

        /////////////////
        BYTE       *currentBuf = Buffer;
        ULONG       length = BufferLength;
        while (length > 0)
        {
            ULONG runWrite = min(length, m_WaveDataQueue.ulLength - m_WaveDataQueue.ulReadPtr);
            
            // Copy the wave buffer data to OS buffer
            RtlCopyMemory(currentBuf, m_WaveDataQueue.pWavData + m_WaveDataQueue.ulReadPtr, runWrite);
            // Zero out the wave buffer, so that if wave end of file is reached we should copy only zeros
            RtlZeroMemory(m_WaveDataQueue.pWavData + m_WaveDataQueue.ulReadPtr, runWrite);
            // Update the read pointer
            m_WaveDataQueue.ulReadPtr = (m_WaveDataQueue.ulReadPtr  + runWrite) % m_WaveDataQueue.ulLength;
            currentBuf += runWrite;
            length = length - runWrite;
        }

        ULONG curChunk = (m_WaveDataQueue.ulReadPtr*NUM_OF_CHUNK_FOR_FILE_READ) / m_WaveDataQueue.ulLength;

        if (curChunk != prevChunk)
        {
            m_WaveDataQueue.sChunkDescriptor[prevChunk].bIsChunkEmpty = true;
            if (!m_WaveDataQueue.bEofReached)
            {
                ReadWavChunk(&m_WaveDataQueue.sChunkDescriptor[prevChunk]);
            }
        }
    }
}

/*++
Routine Description:
    - Just a high level read buffer call.

 Arguments:
     Buffer - Pointer to the OS buffer
     BufferLength - Length of the data to be filled (in bytes)
--*/

VOID CWaveReader::ReadWaveData
(
    _Out_writes_bytes_(BufferLength) BYTE       *Buffer, 
    _In_                             ULONG      BufferLength
)
{
    if (m_Mute)
    {
        RtlZeroMemory(Buffer, BufferLength);
    }
    else
    {
        CopyDataFromRingBuffer(Buffer, BufferLength);
    }
}

/*++
Routine Description:
- initialization for the wavereader member variables, 
- Allocating memory for the 1 second buffer
- Preread the one second buffer data, so that when OS comes to read the data we have it available in the memory.

Arguments:
    WfExt - Format which should be used for capture
    fileNameString - name of the file to be read

Return:
    NTStatus
--*/
#pragma code_seg("PAGE")
NTSTATUS CWaveReader::Init
(
    _In_    PWAVEFORMATEXTENSIBLE   WfExt,
    _In_    PUNICODE_STRING puiFileName,
    _In_    ULONG  loopCount
)
{
    PAGED_CODE();
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    KFLOATING_SAVE  saveData;
    
    // Save floating state (just in case).
    ntStatus = KeSaveFloatingPointState(&saveData);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    if (loopCount < 1)
    {
        // We cannot make loopcount less than 1. By mistake if we make it zero. We will overwrite it with 1.
        m_LoopCount = 1;
    }
    else
    {
        m_LoopCount = loopCount;
    }

    //
    // This sample supports PCM 16bit formats only. 
    //
    if ((WfExt->Format.wFormatTag != WAVE_FORMAT_PCM &&
        !(WfExt->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
          IsEqualGUIDAligned(WfExt->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))) ||
        (WfExt->Format.wBitsPerSample != 16 &&
         WfExt->Format.wBitsPerSample != 8))
    {
        ntStatus = STATUS_NOT_SUPPORTED;
    }
    IF_FAILED_JUMP(ntStatus, Done);

    // Basic init.
    m_ChannelCount      = WfExt->Format.nChannels;      // # channels.
    m_BitsPerSample     = WfExt->Format.wBitsPerSample; // bits per sample.
    m_SamplesPerSecond  = WfExt->Format.nSamplesPerSec; // samples per sec.
    m_Mute              = false;

    // Wave data queue initialization
    m_WaveDataQueue.ulLength = WfExt->Format.nAvgBytesPerSec;
    m_WaveDataQueue.bEofReached = false;
    m_WaveDataQueue.ulReadPtr = 0;
    m_WaveDataQueue.firstChunkIsRead = false;

    // Mark all the chunk empty
    for (int i = 0; i < NUM_OF_CHUNK_FOR_FILE_READ; i++)
    {
        m_WaveDataQueue.sChunkDescriptor[i].bIsChunkEmpty = true;
        m_WaveDataQueue.sChunkDescriptor[i].chunkNumber = (WORD)i;
    }

    ntStatus = OpenWaveFile(puiFileName);
    IF_FAILED_JUMP(ntStatus, Done);

    ntStatus = AllocateBigBuffer();
    IF_FAILED_JUMP(ntStatus, Done);

    ntStatus = ReadHeaderAndFillBuffer();

Done:
    ntStatus = KeRestoreFloatingPointState(&saveData);
    return ntStatus;
}

/*++
Routine Description:
    This function read the wave header file and compare the header info with the
    stream info. Currently we are using only number of channel, sampling frequency 
    and bits per sample as the primary parameters for the wave file to compare against
    stream params. If the params don't match we return success but streams zeros.

Return:
    NTStatus
--*/

NTSTATUS CWaveReader::ReadHeaderAndFillBuffer()
{
    PAGED_CODE();
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    ntStatus = FileReadHeader();

    if(NT_SUCCESS(ntStatus))
    {
        if (m_WaveHeader.numChannels != m_ChannelCount ||
            m_WaveHeader.bitsPerSample != m_BitsPerSample ||
            m_WaveHeader.sampleRate != m_SamplesPerSecond)
        {
            // If the wave file format don't match we wont treat this as error
            // and we will stream zeros. So we return from here and will not read the 
            // wave file and wont fill the buffers.
            return STATUS_SUCCESS;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        // If the wave file format is same as the stream format we will stream the data 
        //  else we will just stream zeros.
        ReadWavChunk(&m_WaveDataQueue.sChunkDescriptor[0]); // Fill the first chunk
        ReadWavChunk(&m_WaveDataQueue.sChunkDescriptor[1]);  // Fill the second chunk
        // The above two calls are scheduling two workitem in the workitemqueue, so that 
        // they gets picked one after the other and fill the first chunk and the second chunk respectively.
        // The problem  that, sometime the second chunk is scheduled ahead of first chunk.
        // To fix this problem we have set the initial read pointer to the start of the chunk,
        // which gets scheduled first. search for "initial worktiem scheduling issue in this file"
    }

    return ntStatus;
}

/*++
Routine Description:
    This function allocates 1 second buffer.
    Segments the buffer into multiple (currently two) chunks. Assigns the start pointer and length
    for each chunk.

Return:
    NTStatus
--*/

NTSTATUS CWaveReader::AllocateBigBuffer()
{
    PAGED_CODE();
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    m_WaveDataQueue.pWavData = (PBYTE)
        ExAllocatePoolWithTag
        (
            NonPagedPoolNx,
            m_WaveDataQueue.ulLength,
            WAVE_DATA_BUFFER_TAG
        );
    if (!m_WaveDataQueue.pWavData)
    {
        DPF(D_TERSE, ("[Could not allocate memory for reading data from wave file]"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        RtlZeroMemory(m_WaveDataQueue.pWavData, m_WaveDataQueue.ulLength);

        ULONG chunklLength = m_WaveDataQueue.ulLength / NUM_OF_CHUNK_FOR_FILE_READ;
        for (int i = 0; i < NUM_OF_CHUNK_FOR_FILE_READ; i++)
        {
            m_WaveDataQueue.sChunkDescriptor[i].pStartAddress = m_WaveDataQueue.pWavData + chunklLength*i;
            m_WaveDataQueue.sChunkDescriptor[i].ulChunkLength = chunklLength;
        }
    }
    return ntStatus;
}

/*++
Routine Description:
    This function opens wave file.

Arguments:
    fileNameString - Name of the wave file

Return:
    NTStatus
--*/

NTSTATUS CWaveReader::OpenWaveFile(PUNICODE_STRING puiFileName)
{
    PAGED_CODE();
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    if (NT_SUCCESS(ntStatus) && puiFileName->Buffer != NULL)
    {
        // Create data file.
        InitializeObjectAttributes
        (
            &m_objectAttributes,
            puiFileName,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL
        );

        // Open Wave File
        ntStatus = FileOpen();
    }

    return ntStatus;
}

/*++
Routine Description:
    This function closes wave file handle.

Return:
    NTStatus
--*/

NTSTATUS CWaveReader::FileClose()
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_SUCCESS;

    if (m_FileHandle)
    {
        ntStatus = ZwClose(m_FileHandle);
        m_FileHandle = NULL;
    }

    return ntStatus;
} 

/*++
Routine Description:
    Reads the wave file file header information

Return:
    NTStatus
--*/

NTSTATUS CWaveReader::FileReadHeader()
{
    PAGED_CODE();
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK             ioStatusBlock;


    ntStatus = ZwReadFile(m_FileHandle,
                        NULL, 
                        NULL, 
                        NULL,
                        &ioStatusBlock,
                        &m_WaveHeader,
                        sizeof(WAVEHEADER), 
                        NULL, 
                        NULL);

    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_TERSE, ("[CWaveReader::FileReadHeader : Error reading wave file]"));
    }
    return ntStatus;
}

/*++
Routine Description:
    This function opens wave file.

Return:
    NTStatus
--*/

NTSTATUS CWaveReader::FileOpen()
{
 
    PAGED_CODE();
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK             ioStatusBlock;

    if (!m_FileHandle)
    {
        ntStatus =
            ZwCreateFile
            (
                &m_FileHandle,
                GENERIC_READ,
                &m_objectAttributes,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
            );

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_TERSE, ("[CWaveReader::FileOpen : Error opening wave file]"));
        }
    }

    return ntStatus;
} 
