/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    queue.cpp

Abstract:

    This file implements the I/O queue interface and performs
    the read/write/ioctl operations.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/


#include "internal.h"

//
// IUnknown implementation
//

//
// Queue destructor.
// Free up the buffer, wait for thread to terminate and 
// delete critical section.
//


CMyQueue::~CMyQueue(
    VOID
    )
/*++

Routine Description:


    IUnknown implementation of Release

Arguments:


Return Value:

    ULONG (reference count after Release)

--*/
{
    if (m_Buffer) {
        delete [] m_Buffer;
    }

    if (m_InitCritSec) {
        ::DeleteCriticalSection(&m_Crit);
    }
}


//
// Initialize 
HRESULT 
CMyQueue::CreateInstance(
    _In_ IWDFDevice *FxDevice,
    _Out_ PCMyQueue *Queue
    )
/*++

Routine Description:


    CreateInstance creates an instance of the queue object.

Arguments:
    
    ppUkwn - OUT parameter is an IUnknown interface to the queue object

Return Value:

    HRESULT indicating success or failure

--*/
{
    CMyQueue *pMyQueue = new CMyQueue;
    HRESULT hr;

    if (pMyQueue == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pMyQueue->Initialize(FxDevice);

    if (SUCCEEDED(hr)) 
    {
        *Queue = pMyQueue;
    }
    else
    {
        pMyQueue->Release();
    }
    return hr;
}

HRESULT
CMyQueue::Initialize(
    _In_ IWDFDevice *FxDevice
    )
{
    IWDFIoQueue *fxQueue;
    HRESULT hr;

    //
    // Initialize the critical section before we continue
    //

    if (!InitializeCriticalSectionAndSpinCount(&m_Crit,0x80000400))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    m_InitCritSec = TRUE;

    //
    // Create the framework queue
    //

    {
        IUnknown *unknown = QueryIUnknown();
        hr = FxDevice->CreateIoQueue(unknown,
                                     TRUE,
                                     WdfIoQueueDispatchSequential,
                                     TRUE,
                                     FALSE,
                                     &fxQueue);
        unknown->Release();
    }

    if (FAILED(hr))
    {
        goto Exit;
    }

    m_FxQueue = fxQueue;

    fxQueue->Release();

Exit:
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CMyQueue::QueryInterface(
    _In_ REFIID InterfaceId,
    _Out_ PVOID *Object
    )
/*++

Routine Description:


    Query Interface

Arguments:
    
    Follows COM specifications

Return Value:

    HRESULT indicating success or failure

--*/
{
    HRESULT hr;


    if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackWrite))) {
        *Object = QueryIQueueCallbackWrite(); 
        hr = S_OK;
    } else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackRead))) {
        *Object = QueryIQueueCallbackRead();
        hr = S_OK;
    } else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackDeviceIoControl))) {
        *Object = QueryIQueueCallbackDeviceIoControl();
        hr = S_OK;
    } else {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

VOID
STDMETHODCALLTYPE
CMyQueue::OnDeviceIoControl(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ ULONG ControlCode,
    _In_ SIZE_T InputBufferSizeInBytes,
    _In_ SIZE_T OutputBufferSizeInBytes
    )
/*++

Routine Description:


    DeviceIoControl dispatch routine

Arguments:
    
    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    ControlCode - IO Control Code
    InputBufferSizeInBytes - Length of input buffer
    OutputBufferSizeInBytes - Length of output buffer

    Always succeeds DeviceIoIoctl
Return Value:

    VOID

--*/
{

    UNREFERENCED_PARAMETER(pWdfQueue);
    UNREFERENCED_PARAMETER(ControlCode);
    UNREFERENCED_PARAMETER(InputBufferSizeInBytes);
    UNREFERENCED_PARAMETER(OutputBufferSizeInBytes);

    pWdfRequest->Complete(S_OK);
    return;
}

VOID
STDMETHODCALLTYPE
CMyQueue::OnWrite(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ SIZE_T BytesToWrite
    )
/*++

Routine Description:


    Write dispatch routine
    IQueueCallbackWrite

Arguments:
    
    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    BytesToWrite - Length of bytes in the write buffer

    Allocate and copy data to local buffer
Return Value:

    VOID

--*/
{

    HRESULT     hr;
    IWDFMemory* pRequestMemory = NULL;
    IWDFIoRequest2 * pWdfRequest2 = NULL;

    UNREFERENCED_PARAMETER(pWdfQueue);

    //
    // Handle Zero length writes.
    //

    if (!BytesToWrite) {
        pWdfRequest->CompleteWithInformation(S_OK, 0);
        return;
    }

    if( BytesToWrite > MAX_WRITE_LENGTH ) {

        pWdfRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_MORE_DATA), 0);
        return;
    }

    // Release previous buffer if set

    if( m_Buffer != NULL ) {
        delete [] m_Buffer;
        m_Buffer = NULL;
        m_Length = 0L;
    }

    // Allocate Buffer

    m_Buffer = new UCHAR[BytesToWrite]; 
    if (m_Buffer == NULL) {
        pWdfRequest->Complete(E_OUTOFMEMORY);
        m_Length = 0L;
        return;
    }

    // Get memory object
    hr = pWdfRequest->QueryInterface(IID_PPV_ARGS(&pWdfRequest2));
    
    if (FAILED(hr)) {
        goto Exit;
    }
    
    hr = pWdfRequest2->RetrieveInputMemory(&pRequestMemory);

    if (FAILED(hr)) {
        goto Exit;
    }

    // Copy from memory object to our buffer

    hr = pRequestMemory->CopyToBuffer(0, m_Buffer, BytesToWrite);

    if (FAILED(hr)) {
        goto Exit;
    }

    //
    // Release memory object.
    //
    SAFE_RELEASE(pRequestMemory);

    //
    // Save the information so that we can use it 
    // to complete the request later.
    //

    Lock();

    m_Length = (ULONG) BytesToWrite;
    m_XferredBytes = m_Length;
    m_CurrentRequest = pWdfRequest2;

    Unlock();

Exit:

    if (FAILED(hr)) {
        if (pWdfRequest2) {
            pWdfRequest2->CompleteWithInformation(hr, 0);
        }
        delete [] m_Buffer;
        m_Buffer = NULL;
        SAFE_RELEASE(pRequestMemory);
    }

    //
    // This is an early release. pWdfRequest2 will be released, when the request is completed
    //
    SAFE_RELEASE(pWdfRequest2);

    return;
}

VOID
STDMETHODCALLTYPE
CMyQueue::OnRead(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ SIZE_T SizeInBytes
    )
/*++

Routine Description:


    Read dispatch routine
    IQueueCallbackRead

Arguments:
    
    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    SizeInBytes - Length of bytes in the read buffer

    Copy available data into the read buffer
Return Value:

    VOID

--*/
{
    IWDFMemory* pRequestMemory = NULL;
    IWDFIoRequest2 * pWdfRequest2 = NULL;
    HRESULT     hr;

    UNREFERENCED_PARAMETER(pWdfQueue);

    //
    // Handle Zero length reads.
    //

    if (!SizeInBytes) {
        pWdfRequest->CompleteWithInformation(S_OK, 0);
        return;
    }

    if (m_Buffer == NULL) {
        pWdfRequest->CompleteWithInformation(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), SizeInBytes);
        return;
    }

    if (m_Length < SizeInBytes) {
        SizeInBytes = m_Length;
    }

    //
    // Get memory object
    //
    
    hr = pWdfRequest->QueryInterface(IID_PPV_ARGS(&pWdfRequest2));

    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pWdfRequest2->RetrieveOutputMemory(&pRequestMemory );

    if (FAILED(hr)) {
        goto Exit;
    }

    // Copy from buffer to memory object 

    hr = pRequestMemory->CopyFromBuffer(0, m_Buffer, SizeInBytes);

    if (FAILED(hr)) {
        goto Exit;
    }

    //
    // Release memory object.
    //

    SAFE_RELEASE(pRequestMemory);

    //
    // Save the information so that we can use it 
    // to complete the request later.
    //

    Lock();

    m_CurrentRequest = pWdfRequest2;
    m_XferredBytes = SizeInBytes;

    Unlock();

Exit:
     
    if (FAILED(hr)) {
        if (pWdfRequest2) {
            pWdfRequest2->CompleteWithInformation(hr, 0);
        }
        SAFE_RELEASE(pRequestMemory);
    }

    //
    // This is an early release. pWdfRequest2 will be released, when the request is completed
    //
    SAFE_RELEASE(pWdfRequest2);
    
    return;
}

DWORD 
CMyQueue::CompletionThread(
    PVOID ThreadParameter
    )
/*++

Routine Description:


    This routine is called from the thread started to complete
    I/O requests. It sleeps for TIMER_PERIOD and then completes
    the current request. Note that it has to release the lock
    before it calls the request complete method. 

Arguments:
    
    ThreadParameter - This is a pointer to the Queue object.
    
Return Value:

    VOID

--*/
{
    CMyQueue      *pQueue = (CMyQueue *)ThreadParameter;
    IWDFIoRequest2 *request;
    SIZE_T       bytesXferred = 0;

    for (;;) {

        //
        // Block for a fixed time and then complete the request.
        //

        Sleep(TIMER_PERIOD);

        pQueue->Lock();

        //
        // Process the current request.
        //

        request = pQueue->m_CurrentRequest;

        if (request) {
            bytesXferred = pQueue->m_XferredBytes;
        }

        // 
        // Reset values.
        //

        pQueue->m_CurrentRequest = NULL;
        pQueue->m_XferredBytes = 0;


        pQueue->Unlock();

        if (request) {
            request->CompleteWithInformation(S_OK, bytesXferred);
        }

        //
        // If thread needs to be terminated
        //

        if (pQueue->m_ExitThread) {
            ExitThread(0);
        }

    }

}
