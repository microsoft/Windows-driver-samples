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
#include "ReadWriteQueue.tmh"

VOID
CMyReadWriteQueue::OnCompletion(
    _In_ IWDFIoRequest*                 pWdfRequest,
    _In_ IWDFIoTarget*                  pIoTarget,
    _In_ IWDFRequestCompletionParams*   pParams,
    _In_ PVOID                          pContext
    )
{
    UNREFERENCED_PARAMETER(pIoTarget);
    UNREFERENCED_PARAMETER(pContext);

    pWdfRequest->CompleteWithInformation(
        pParams->GetCompletionStatus(),
        pParams->GetInformation()
        );
}

void
CMyReadWriteQueue::ForwardFormattedRequest(
    _In_ IWDFIoRequest*                         pRequest,
    _In_ IWDFIoTarget*                          pIoTarget
    )
{
    //
    //First set the completion callback
    //

    IRequestCallbackRequestCompletion * pCompletionCallback = NULL;
    HRESULT hrQI = this->QueryInterface(IID_PPV_ARGS(&pCompletionCallback));
    WUDF_TEST_DRIVER_ASSERT(SUCCEEDED(hrQI) && (NULL != pCompletionCallback));

    pRequest->SetCompletionCallback(
        pCompletionCallback,
        NULL
        );

    pCompletionCallback->Release();
    pCompletionCallback = NULL;

    //
    //Send down the request
    //

    HRESULT hrSend = S_OK;
    hrSend = pRequest->Send(pIoTarget,
                            0,  //flags
                            0); //timeout

    if (FAILED(hrSend))
    {
        pRequest->CompleteWithInformation(hrSend, 0);
    }

    return;
}


CMyReadWriteQueue::CMyReadWriteQueue(
    _In_ PCMyDevice Device
    ) :
    CMyQueue(Device)
{
}

//
// Queue destructor.
// Free up the buffer, wait for thread to terminate and
//

CMyReadWriteQueue::~CMyReadWriteQueue(
    VOID
    )
/*++

Routine Description:


    IUnknown implementation of Release

Aruments:


Return Value:

    ULONG (reference count after Release)

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION,
                TEST_TRACE_QUEUE,
                "%!FUNC! Entry"
                );

}


HRESULT
STDMETHODCALLTYPE
CMyReadWriteQueue::QueryInterface(
    _In_ REFIID InterfaceId,
    _Outptr_ PVOID *Object
    )
/*++

Routine Description:


    Query Interface

Aruments:

    Follows COM specifications

Return Value:

    HRESULT indicatin success or failure

--*/
{
    HRESULT hr;


    if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackWrite)))
    {
        hr = S_OK;
        *Object = QueryIQueueCallbackWrite();
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackRead)))
    {
        hr = S_OK;
        *Object = QueryIQueueCallbackRead();
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IRequestCallbackRequestCompletion)))
    {
        hr = S_OK;
        *Object = QueryIRequestCallbackRequestCompletion();
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackIoStop)))
    {
        hr = S_OK;
        *Object = QueryIQueueCallbackIoStop();
    }
    else
    {
        hr = CMyQueue::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

//
// Initialize
//

HRESULT
CMyReadWriteQueue::CreateInstance(
    _In_ PCMyDevice Device,
    _Out_ PCMyReadWriteQueue *Queue
    )
/*++

Routine Description:


    CreateInstance creates an instance of the queue object.

Aruments:

    ppUkwn - OUT parameter is an IUnknown interface to the queue object

Return Value:

    HRESULT indicatin success or failure

--*/
{
    PCMyReadWriteQueue queue;
    HRESULT hr = S_OK;

    queue = new CMyReadWriteQueue(Device);

    if (NULL == queue)
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Call the queue callback object to initialize itself.  This will create
    // its partner queue framework object.
    //

    if (SUCCEEDED(hr))
    {
        hr = queue->Initialize();
    }

    if (SUCCEEDED(hr))
    {
        *Queue = queue;
    }
    else
    {
        SAFE_RELEASE(queue);
    }

    return hr;
}

HRESULT
CMyReadWriteQueue::Initialize(
    )
{
    HRESULT hr;

    //
    // First initialize the base class.  This will create the partner FxIoQueue
    // object and setup automatic forwarding of I/O controls.
    //

    hr = __super::Initialize(WdfIoQueueDispatchParallel,
                             true,
                             true);

    //
    // return the status.
    //

    return hr;
}

STDMETHODIMP_ (void)
CMyReadWriteQueue::OnWrite(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ SIZE_T BytesToWrite
     )
/*++

Routine Description:


    Write dispatch routine
    IQueueCallbackWrite

Aruments:

    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    BytesToWrite - Lenth of bytes in the write buffer

    Allocate and copy data to local buffer
Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(pWdfQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION,
                TEST_TRACE_QUEUE,
                "%!FUNC!: Queue %p Request %p BytesToTransfer %d\n",
                this,
                pWdfRequest,
                (ULONG)(ULONG_PTR)BytesToWrite
                );

    HRESULT hr = S_OK;
    IWDFMemory * pInputMemory = NULL;
    IWDFUsbTargetPipe * pOutputPipe = m_Device->GetOutputPipe();

    pWdfRequest->GetInputMemory(&pInputMemory);

    hr = pOutputPipe->FormatRequestForWrite(
                                pWdfRequest,
                                NULL, //pFile
                                pInputMemory,
                                NULL, //Memory offset
                                NULL  //DeviceOffset
                                );

    if (FAILED(hr))
    {
        pWdfRequest->Complete(hr);
    }
    else
    {
        ForwardFormattedRequest(pWdfRequest, pOutputPipe);
    }

    SAFE_RELEASE(pInputMemory);

    return;
}

STDMETHODIMP_ (void)
CMyReadWriteQueue::OnRead(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ SIZE_T BytesToRead
    )
/*++

Routine Description:


    Read dispatch routine
    IQueueCallbackRead

Aruments:

    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    BytesToRead - Lenth of bytes in the read buffer

    Copy available data into the read buffer
Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(pWdfQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION,
                TEST_TRACE_QUEUE,
                "%!FUNC!: Queue %p Request %p BytesToTransfer %d\n",
                this,
                pWdfRequest,
                (ULONG)(ULONG_PTR)BytesToRead
                );

    HRESULT hr = S_OK;
    IWDFMemory * pOutputMemory = NULL;

    pWdfRequest->GetOutputMemory(&pOutputMemory);

    hr = m_Device->GetInputPipe()->FormatRequestForRead(
                                pWdfRequest,
                                NULL, //pFile
                                pOutputMemory,
                                NULL, //Memory offset
                                NULL  //DeviceOffset
                                );

    if (FAILED(hr))
    {
        pWdfRequest->Complete(hr);
    }
    else
    {
        ForwardFormattedRequest(pWdfRequest, m_Device->GetInputPipe());
    }

    SAFE_RELEASE(pOutputMemory);

    return;
}

STDMETHODIMP_ (void)
CMyReadWriteQueue::OnIoStop(
    _In_ IWDFIoQueue *   pWdfQueue,
    _In_ IWDFIoRequest * pWdfRequest,
    _In_ ULONG           ActionFlags
    )
{   
    UNREFERENCED_PARAMETER(pWdfQueue);


    //
    // The driver owns the request and no locking constraint is safe for 
    // the queue callbacks
    //        
    if (ActionFlags == WdfRequestStopActionSuspend )
    {
        IWDFIoRequest2 * request2 = NULL;
        HRESULT                 hr;

        hr = pWdfRequest->QueryInterface(IID_PPV_ARGS(&request2));
        if (FAILED(hr))
        {
            TraceEvents(TRACE_LEVEL_INFORMATION,
                        TEST_TRACE_QUEUE,
                        "%!FUNC!: Failed to QI for IWDFIoRequest2: %!hresult!", 
                        hr);
            return;
        }

        request2->StopAcknowledge(FALSE); //don't requeue
        SAFE_RELEASE(request2);
    }
    else if(ActionFlags == WdfRequestStopActionPurge)
    {
        //
        // Cancel the sent request since we are asked to purge the request
        //

        pWdfRequest->CancelSentRequest();
    }

    return;
}

