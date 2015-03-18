/*++
 
Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Queue.cpp

Abstract:

    This module contains the implementation of the OSR USB Filter Sample driver's
    queue callback object.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "queue.h"

#include "queue.tmh"

HRESULT
CMyQueue::CreateInstance(
    _In_ IWDFDevice * FxDevice,
    _Out_ CMyQueue **Queue
    )
/*++
 
  Routine Description:

    This method creates and initializs an instance of the OSR USB Filter Sample driver's 
    device callback object.

  Arguments:

    FxDeviceInit - the settings for the device.

    Device - a location to store the referenced pointer to the device object.

  Return Value:

    Status

--*/
{
    CMyQueue *queue;

    HRESULT hr = S_OK;

    //
    // Allocate a new instance of the device class.
    //

    queue = new CMyQueue();

    if (NULL == queue)
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Initialize the instance.
    //

    if (SUCCEEDED(hr)) 
    {
        hr = queue->Initialize(FxDevice);
    }

    if (SUCCEEDED(hr)) 
    {
        queue->AddRef();
        *Queue = queue;
    }

    if (NULL != queue)
    {
        queue->Release();
    }

    return hr;
}

HRESULT
CMyQueue::QueryInterface(
    _In_ REFIID InterfaceId,
    _Out_ PVOID *Object
    )
/*++
 
  Routine Description:

    This method is called to get a pointer to one of the object's callback
    interfaces.  

  Arguments:

    InterfaceId - the interface being requested

    Object - a location to store the interface pointer if successful

  Return Value:

    S_OK or E_NOINTERFACE

--*/
{
    HRESULT hr;


    if(IsEqualIID(InterfaceId, __uuidof(IQueueCallbackDefaultIoHandler))) 
    {
        hr = S_OK;
        *Object = QueryIQueueCallbackDefaultIoHandler();
    } 
    else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackWrite))) 
    {
        hr = S_OK;
        *Object = QueryIQueueCallbackWrite(); 
    } 
    else if (IsEqualIID(InterfaceId, __uuidof(IRequestCallbackRequestCompletion))) 
    {
        hr = S_OK;
        *Object = QueryIRequestCallbackRequestCompletion();

    } 
    else 
    {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

HRESULT
CMyQueue::Initialize(
    _In_ IWDFDevice *FxDevice
    )
/*++
 
  Routine Description:

    This method initializes the device callback object.  Any operations which
    need to be performed before the caller can use the callback object, but 
    which couldn't be done in the constructor becuase they could fail would
    be placed here.

  Arguments:

    FxDevice - the device which this Queue is for.

  Return Value:

    status.

--*/
{
    IWDFIoQueue     *fxQueue;
    HRESULT hr;

    //
    // Create the framework queue
    //

    IUnknown *unknown = QueryIUnknown();
    hr = FxDevice->CreateIoQueue(
                        unknown,
                        TRUE,                        // bDefaultQueue
                        WdfIoQueueDispatchParallel, 
                        FALSE,                       // bPowerManaged
                        TRUE,                        // bAllowZeroLengthRequests
                        &fxQueue 
                        );
    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            "%!FUNC!: Could not create default I/O queue, %!hresult!",
            hr
            );
    }
    
    unknown->Release();

    if (SUCCEEDED(hr)) 
    {
        m_FxQueue = fxQueue;

        //
        // m_FxQueue is kept as a Weak reference to framework Queue object to avoid 
        // circular reference. This object's lifetime is contained within 
        // framework Queue object's lifetime
        //
        
        fxQueue->Release();
    }

    if (SUCCEEDED(hr)) 
    {
         FxDevice->GetDefaultIoTarget(&m_FxIoTarget);        
    }

    return hr;
}

void
CMyQueue::InvertBits(
    _Inout_ IWDFMemory* FxMemory,
    _In_    SIZE_T      NumBytes
    )
/*++
 
  Routine Description:

    This helper method inverts bits in the buffer of an FxMemory object

  Arguments:

    FxMemory - Framework memory object whose buffer's bits are to be inverted

    NumBytes - Number of bytes for which bits are to be inverted

  Return Value:

    None

--*/
{
    PBYTE Buffer = (PBYTE) 
        FxMemory->GetDataBuffer(NULL);

    for (SIZE_T i = 0; i < NumBytes; i++)
    {
        memset(Buffer + i, ~(Buffer[i]), sizeof(*Buffer));
    }    
}

void
CMyQueue::OnWrite(
    _In_ IWDFIoQueue*   FxQueue,
    _In_ IWDFIoRequest* FxRequest,        
    _In_ SIZE_T         NumOfBytesToWrite
    )
/*++
 
  Routine Description:

    This method is called by Framework Queue object to deliver the Write request
    This method inverts the bits in write buffer and forwards the request down the device stack
    In case of any failure prior to ForwardRequest, it completets the request with failure

  Arguments:

    pWdfQueue - Framework Queue which is delivering the request

    pWdfRequest - Framework Request

  Return Value:

    None

--*/    
{
    UNREFERENCED_PARAMETER(FxQueue);

    IWDFMemory * FxInputMemory = NULL;
    
    FxRequest->GetInputMemory(&FxInputMemory);
    
    //
    // Invert bits of the buffer to be written to device
    //
    
    InvertBits(FxInputMemory, NumOfBytesToWrite);

    //
    // Forward request down the stack
    // When the device below completes the request we will get notified in OnComplete
    // and then we will complete the request
    //
    
    ForwardRequest(FxRequest);

    FxInputMemory->Release();
}

//
// IQueueCallbackDefaultIoHandler method
//

void
CMyQueue::OnDefaultIoHandler(
    _In_ IWDFIoQueue*    FxQueue,
    _In_ IWDFIoRequest*  FxRequest
    )
/*++
 
  Routine Description:

    This method is called by Framework Queue object to deliver all the I/O
    Requests for which we do not have a specific handler
    (In our case anything other than Write)

  Arguments:

    pWdfQueue - Framework Queue which is delivering the request

    pWdfRequest - Framework Request

  Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER(FxQueue);
    
    //
    // We just forward the request down the stack
    // When the device below completes the request we will get notified in OnComplete
    // and then we will complete the request
    //
    
    ForwardRequest(FxRequest);
}

void
CMyQueue::ForwardRequest(
    _In_ IWDFIoRequest* FxRequest
    )
/*++
 
  Routine Description:

    This helper method forwards the request down the stack

  Arguments:

    pWdfRequest - Request to be forwarded

  Return Value:

    None

  Remarks:

    The request gets forwarded to the next device in the stack which can be:
        1. Next device in user-mode stack
        2. Top device in kernel-mode stack (Redirector's Down Device)

    In this routine we:
        1. Set a completion callback
        2. Copy request parameters to next stack location
        3. Asynchronously send the request without any timeout

        When the lower request gets completed we will be notified via the
        completion callback, where we will complete our request

    In case of failure this routine completes the request            

--*/
{
    //
    //First set the completion callback
    //

    IRequestCallbackRequestCompletion *completionCallback = 
        QueryIRequestCallbackRequestCompletion();
        
    FxRequest->SetCompletionCallback(
        completionCallback,
        NULL                    //pContext
        );

    completionCallback->Release();
    
    //
    //Copy current i/o stack locations parameters to the next stack location
    //
    
    FxRequest->FormatUsingCurrentType(
        );

    //
    //Send down the request
    //
    HRESULT hrSend = S_OK;
    
    hrSend = FxRequest->Send(
                m_FxIoTarget,
                0,              //No flag
                0               //No timeout
                );
    
    if (FAILED(hrSend))
    {
        //
        //If send failed we need to complete the request with failure
        //
        FxRequest->CompleteWithInformation(hrSend, 0);
    }

    return;        
}

void
CMyQueue::HandleReadRequestCompletion(
    IWDFIoRequest*                 FxRequest,
    IWDFIoRequestCompletionParams* CompletionParams
    )
/*++
 
  Routine Description:

    This helper method is called by OnCompletion method to complete Read request 
    We invert the bits in the read buffer
    This is so that the client reads back the data it wrote since
    we inverted bits during write to device

  Arguments:

    FxRequest - Request object of our layer

    CompletionParams - Parameters with which the lower Request got completed

  Return Value:

    None

  Remarks:

    This method always completes the request since no one else would get a chance to
    complete the request
    In case of failure it completes the request with failure
    
--*/
{
    HRESULT hrCompletion = CompletionParams->GetCompletionStatus();
    ULONG_PTR BytesRead = CompletionParams->GetInformation();

    //
    // Check 
    //  1. whether the lower device succeeded the Request (otherwise we will just complete
    //     the Request with failure
    //  2. If data read is of non-zero length, for us to bother to invert its bits
    //
    
    if (SUCCEEDED(hrCompletion) &&
        (0 != BytesRead)
        )
    {
        IWDFMemory  *FxOutputMemory;
        
        FxRequest->GetOutputMemory(&FxOutputMemory );

        InvertBits(FxOutputMemory, BytesRead);
        
        FxOutputMemory->Release();
    }

    //
    // Complete the request
    //
    
    FxRequest->CompleteWithInformation(
        hrCompletion,
        BytesRead
        );    
}
    

void
CMyQueue::OnCompletion(
    IWDFIoRequest*                 FxRequest,
    IWDFIoTarget*                  FxIoTarget,
    IWDFRequestCompletionParams*   CompletionParams,
    PVOID                          Context
    )
/*++
 
  Routine Description:

    This method is called by Framework I/O Target object when 
    the lower device completets the Request

  Arguments:

    pWdfRequest - Request object of our layer

    pIoTarget - I/O Target object invoking this callback

    pParams - Parameters with which the lower Request got completed

  Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER(FxIoTarget);
    UNREFERENCED_PARAMETER(Context);

    //
    // If it is a read request, we invert the bits read since we inverted them during write
    // so that application would read the same data as it wrote
    //

    if (WdfRequestRead == FxRequest->GetType())
    {
        IWDFIoRequestCompletionParams * IoCompletionParams = NULL;
        HRESULT hrQI = CompletionParams->QueryInterface(IID_PPV_ARGS(&IoCompletionParams));
        WUDF_SAMPLE_DRIVER_ASSERT(SUCCEEDED(hrQI));
    
        HandleReadRequestCompletion(
            FxRequest, 
            IoCompletionParams
            );

        SAFE_RELEASE(IoCompletionParams);
    }
    else
    {

        //
        // Otherwise we just complete our Request object with the same parameters
        // with which the lower Request got completed
        //
        
        FxRequest->CompleteWithInformation(
            CompletionParams->GetCompletionStatus(),
            CompletionParams->GetInformation()
            );    
    }
}

