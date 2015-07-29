/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    savedata.h

Abstract:

    Declaration of SYSVAD data saving class. This class supplies services
to save data to disk.


--*/

#ifndef _SYSVAD_SAVEDATA_H
#define _SYSVAD_SAVEDATA_H

//-----------------------------------------------------------------------------
//  Forward declaration
//-----------------------------------------------------------------------------
class CSaveData;
typedef CSaveData *PCSaveData;


//-----------------------------------------------------------------------------
//  Structs
//-----------------------------------------------------------------------------

// Parameter to workitem.
#include <pshpack1.h>
typedef struct _SAVEWORKER_PARAM {
    PIO_WORKITEM     WorkItem;
    ULONG            ulFrameNo;
    ULONG            ulDataSize;
    PBYTE            pData;
    PCSaveData       pSaveData;
    KEVENT           EventDone;
} SAVEWORKER_PARAM;
typedef SAVEWORKER_PARAM *PSAVEWORKER_PARAM;
#include <poppack.h>

// wave file header.
#include <pshpack1.h>
typedef struct _OUTPUT_FILE_HEADER
{
    DWORD           dwRiff;
    DWORD           dwFileSize;
    DWORD           dwWave;
    DWORD           dwFormat;
    DWORD           dwFormatLength;
} OUTPUT_FILE_HEADER;
typedef OUTPUT_FILE_HEADER *POUTPUT_FILE_HEADER;

typedef struct _OUTPUT_DATA_HEADER
{
    DWORD           dwData;
    DWORD           dwDataLength;
} OUTPUT_DATA_HEADER;
typedef OUTPUT_DATA_HEADER *POUTPUT_DATA_HEADER;

#include <poppack.h>

//-----------------------------------------------------------------------------
//  Classes
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// CSaveData
//   Saves the wave data to disk.
//
IO_WORKITEM_ROUTINE SaveFrameWorkerCallback;

class CSaveData
{
protected:
    UNICODE_STRING              m_FileName;         // DataFile name.
    HANDLE                      m_FileHandle;       // DataFile handle.
    PBYTE                       m_pDataBuffer;      // Data buffer.
    ULONG                       m_ulBufferSize;     // Total buffer size.

    ULONG                       m_ulFrameIndex;     // Current Frame.
    ULONG                       m_ulFrameCount;     // Frame count.
    ULONG                       m_ulFrameSize;
    ULONG                       m_ulBufferOffset;   // index in buffer.
    PBOOL                       m_fFrameUsed;       // Frame usage table.
    KSPIN_LOCK                  m_FrameInUseSpinLock; // Spinlock for synch.
    KMUTEX                      m_FileSync;         // Synchronizes file access

    OBJECT_ATTRIBUTES           m_objectAttributes; // Used for opening file.

    OUTPUT_FILE_HEADER          m_FileHeader;
    PWAVEFORMATEX               m_waveFormat;
    OUTPUT_DATA_HEADER          m_DataHeader;
    PLARGE_INTEGER              m_pFilePtr;

    static PDEVICE_OBJECT       m_pDeviceObject;
    static ULONG                m_ulStreamId;
    static ULONG                m_ulOffloadStreamId;
    static PSAVEWORKER_PARAM    m_pWorkItems;

    BOOL                        m_fWriteDisabled;

    BOOL                        m_bInitialized;

public:
    CSaveData();
    ~CSaveData();

    static NTSTATUS             InitializeWorkItems
    (
        _In_  PDEVICE_OBJECT    DeviceObject
    );
    static void                 DestroyWorkItems
    (
        void
    );
    void                        Disable
    (
        _In_ BOOL               fDisable
    );
    static PSAVEWORKER_PARAM    GetNewWorkItem
    (
        void
    );
    NTSTATUS                    Initialize
    (
        _In_ BOOL               _bOffloaded
    );
	static NTSTATUS             SetDeviceObject
	(
	    _In_  PDEVICE_OBJECT    DeviceObject
	);
	static PDEVICE_OBJECT       GetDeviceObject
	(
	    void
	);
    void                        ReadData
    (
        _Inout_updates_bytes_all_(ulByteCount)  PBYTE   pBuffer,
        _In_                                    ULONG   ulByteCount
    );
    NTSTATUS                    SetDataFormat
    (
        _In_  PKSDATAFORMAT     pDataFormat
    );
    NTSTATUS                    SetMaxWriteSize
    (
        _In_  ULONG             ulMaxWriteSize
    );  
    void                        WaitAllWorkItems
    (
        void
    );
    void                        WriteData
    (
        _In_reads_bytes_(ulByteCount)   PBYTE   pBuffer,
        _In_                            ULONG   ulByteCount
    );

private:
    NTSTATUS                    FileClose
    (
        void
    );
    NTSTATUS                    FileOpen
    (
        _In_  BOOL              fOverWrite
    );
    NTSTATUS                    FileWrite
    (
        _In_reads_bytes_(ulDataSize)    PBYTE   pData,
        _In_                            ULONG   ulDataSize
    );
    NTSTATUS                    FileWriteHeader
    (
        void
    );

    void                        SaveFrame
    (
        _In_ ULONG              ulFrameNo,
        _In_ ULONG              ulDataSize
    );

    friend
    IO_WORKITEM_ROUTINE         SaveFrameWorkerCallback;
};
typedef CSaveData *PCSaveData;

#endif

