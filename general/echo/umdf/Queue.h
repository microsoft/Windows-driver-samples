/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    queue.h

Abstract:

    This file defines the queue callback interface.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

// Set max write length for testing
#define MAX_WRITE_LENGTH (40*1024)

// Set timer period in ms
#define TIMER_PERIOD     100

//
// Queue Callback Object.
//

class CMyQueue : 
    public IQueueCallbackDeviceIoControl,
    public IQueueCallbackRead,
    public IQueueCallbackWrite,
    public CUnknown
{
    PVOID           m_Buffer;           // Current buffer
    ULONG           m_Length;           // Length of the buffer
    SIZE_T          m_XferredBytes;     // Amount of bytes transferred for the current request
    IWDFIoRequest2  *m_CurrentRequest;  // Current request
    CRITICAL_SECTION m_Crit;            // Lock to protect updates to CMyQueue fields
    BOOLEAN         m_ExitThread;       // If TRUE Terminate thread.
    BOOLEAN         m_InitCritSec;      // If TRUE lock initialized

    IWDFIoQueue     *m_FxQueue;

    CMyQueue() : 
        m_Buffer(NULL), 
        m_Length (0),
        m_CurrentRequest(NULL),
        m_XferredBytes(0),
        m_ExitThread(FALSE),
        m_InitCritSec(FALSE),
        m_FxQueue(NULL)
    {
    }

    virtual ~CMyQueue();

    _Acquires_lock_(this->m_Crit)
    __inline 
    void
    Lock(
        )
    {
        ::EnterCriticalSection(&m_Crit);
    }

    _Releases_lock_(this->m_Crit)
    __inline 
    void
    Unlock(
        )
    {
        ::LeaveCriticalSection(&m_Crit);
    }

    HRESULT
    Initialize(
        _In_ IWDFDevice *FxDevice
        );

public:

    //
    // Completion thread routine.
    //

    static DWORD CompletionThread( PVOID ThreadParameter);

    //
    // Sets the flag to make thread exit
    //
    
    void
    SetExitThread()
    {
        m_ExitThread = TRUE;
    }

    static 
    HRESULT 
    CreateInstance( 
        _In_ IWDFDevice *FxDevice,
        _Out_ PCMyQueue *Queue
        );

    HRESULT
    Configure(
        VOID
        )
    {
        return S_OK;
    }

   
    IQueueCallbackDeviceIoControl *
    QueryIQueueCallbackDeviceIoControl(
        VOID
        )
    {
        AddRef();
        return static_cast<IQueueCallbackDeviceIoControl *>(this);
    }
    
    IQueueCallbackRead *
    QueryIQueueCallbackRead(
        VOID
        )
    {
        AddRef();
        return static_cast<IQueueCallbackRead *>(this);
    }

    IQueueCallbackWrite *
    QueryIQueueCallbackWrite(
        VOID
        )
    {
        AddRef();
        return static_cast<IQueueCallbackWrite *>(this);
    }

    //
    // IUnknown
    //

    virtual
    ULONG 
    STDMETHODCALLTYPE
    AddRef(
        VOID
        ) {
        return CUnknown::AddRef();
    }
    
    _At_(this, __drv_freesMem(object))
    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        VOID
        ) {
        return CUnknown::Release();
    }

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        _In_ REFIID InterfaceId, 
        _Out_ PVOID *Object
        );

    //
    // Wdf Callbacks
    //
    
    // IQueueCallbackDeviceIoControl
    //
    virtual
    VOID
    STDMETHODCALLTYPE
    OnDeviceIoControl( 
        _In_ IWDFIoQueue *pWdfQueue,
        _In_ IWDFIoRequest *pWdfRequest,
        _In_ ULONG ControlCode,
        _In_ SIZE_T InputBufferSizeInBytes,
        _In_ SIZE_T OutputBufferSizeInBytes
        );

    // IQueueCallbackWrite
    //
    virtual
    VOID
    STDMETHODCALLTYPE
    OnWrite( 
        _In_ IWDFIoQueue *pWdfQueue,
        _In_ IWDFIoRequest *pWdfRequest,
        _In_ SIZE_T NumOfBytesToWrite
        );

    // IQueueCallbackRead
    //
    virtual
    VOID
    STDMETHODCALLTYPE
    OnRead( 
        _In_ IWDFIoQueue *pWdfQueue,
        _In_ IWDFIoRequest *pWdfRequest,
        _In_ SIZE_T NumOfBytesToRead
        );
};
