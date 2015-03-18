/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    queue.cpp

Abstract:

    This file implements the I/O queue interface and performs
    the read/write/ioctl operations.

Environment:

    user mode only

Revision History:

--*/

#include "internal.h"

#include "queue.tmh"

CMyQueue::CMyQueue(
    ) : 
    m_FxQueue(NULL),
    m_Device(NULL)
{
}

//
// Queue destructor.
//

CMyQueue::~CMyQueue(
    VOID
    )
{
    Trace(
        TRACE_LEVEL_INFORMATION, 
        "%!FUNC!"
        );
}

//
// Initialize 
//

HRESULT
CMyQueue::Initialize(
    _In_ CMyDevice * Device
    )
/*++

Routine Description:

   Queue Initialize helper routine.
   This routine will Create a default parallel queue associated with the Fx device object
   and pass the IUnknown for this queue

Aruments:
    Device - Device object pointer

Return Value:

    S_OK if Initialize succeeds

--*/    
{

    Trace(
        TRACE_LEVEL_INFORMATION, 
        "%!FUNC!"
        );

    CComPtr<IWDFIoQueue> fxQueue;
    
    HRESULT hr;

    m_Device = Device;

    //
    // Create the I/O Queue object.
    //

    {
        CComPtr<IUnknown> pUnk;

        HRESULT hrQI = this->QueryInterface(__uuidof(IUnknown),(void**)&pUnk);
        
        WUDF_SAMPLE_DRIVER_ASSERT(SUCCEEDED(hrQI));

        hr = m_Device->GetFxDevice()->CreateIoQueue(
                                        pUnk,
                                        TRUE,
                                        WdfIoQueueDispatchParallel,
                                        TRUE,
                                        FALSE,
                                        &fxQueue
                                        );
    }

    if (FAILED(hr))
    {
         Trace(
                 TRACE_LEVEL_ERROR, 
                 "Failed to initialize driver queue %!hresult!",
                  hr
                  );
        goto Exit;
    }

    m_FxQueue = fxQueue;
   

Exit:

    return hr;
}

HRESULT
CMyQueue::Configure(
    VOID
    )
/*++

Routine Description:

    Queue configuration function .
    It is called after queue object has been succesfully initialized.
    
Aruments:
    
    NONE

 Return Value:

    S_OK if succeeds. 

--*/    
{
    Trace(
        TRACE_LEVEL_INFORMATION, 
        "%!FUNC!"
        );
    
    HRESULT hr = S_OK;

    return hr;
}


STDMETHODIMP_(void)
CMyQueue::OnCreateFile(
    _In_ IWDFIoQueue* pWdfQueue,
    _In_ IWDFIoRequest* pWdfRequest,
    _In_ IWDFFile* pWdfFileObject
    )

/*++

Routine Description:

    Create callback from the framework for this default parallel queue 
    
    The create request will create a socket connection , create a file i/o target associated 
    with the socket handle for this connection and store in the file object context.

Aruments:
    
    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    pWdfFileObject - WDF file object for this create

 Return Value:

    VOID

--*/
{
    Trace(
        TRACE_LEVEL_INFORMATION, 
        "%!FUNC!"
        );
    
    HRESULT hr = S_OK;

    CComPtr<IWDFFileHandleTargetFactory> spFileHandleTargetFactory;

    CComPtr<IWDFIoTarget> pFileTarget;

    CComPtr<IWDFDevice> pDevice;

    HANDLE SocketHandle = NULL; 

    pWdfQueue->GetDevice(&pDevice);

    FileContext *pContext = NULL;

    //
    // Create new connection object 
    //
    
    CConnection *pConnection = new CConnection();

    if (NULL == pConnection )
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        Trace(
              TRACE_LEVEL_ERROR,
               L"ERROR: Could not create connection object %!hresult!",
               hr
               );
          goto Exit;
    }

    //
    // Connect to the socket server 
    //

    hr = pConnection->Connect(pDevice);

    if (FAILED(hr))
    {
        Trace(
              TRACE_LEVEL_ERROR,
               L"ERROR: Could not connect %!hresult!",
               hr
               );

        goto Exit;       

    }

    //
    // If that succeeds, get socket handle for the connection 
    //

    if ( NULL  ==  (SocketHandle = pConnection->GetSocketHandle()) ) 
    {
        hr = E_FAIL; 
        Trace(
              TRACE_LEVEL_ERROR,
               L"ERROR: Unable to obtain valid Socket Handle %!hresult!",
               hr
               );
          goto Exit;
    }

    //
    // Create file context for this file object 
    //

    pContext  = new  FileContext; 

    if (NULL == pContext)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        Trace(
              TRACE_LEVEL_ERROR,
               L"ERROR: Could not create file context %!hresult!",
               hr
               );
          goto Exit;        

    }

    //
    // QI for IWDFFileHandleTargetFactory from the framework device object. 
    // Note UmdfDispatcher in Wdf Section in the Inf
    //

    hr = pDevice->QueryInterface(IID_PPV_ARGS(&spFileHandleTargetFactory));
    
    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Unable to obtain target factory for creating FileHandle based I/O target %!hresult!",
            hr
            );
        goto Exit;
    }

    //
    // If that succeeds, Create a File Handle I/O Target and associate the socket handle with this target 
    //
    
    hr = spFileHandleTargetFactory->CreateFileHandleTarget(SocketHandle ,&pFileTarget);
 
    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Unable to create framework I/O target %!hresult!",
            hr
            );
        goto Exit;
    }


    pContext->pFileTarget  = pFileTarget;

    pContext->pConnection = pConnection; 
     
    hr = pWdfFileObject->AssignContext(NULL,(void*)pContext);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Unable to Assign Context to this File Object %!hresult!",
            hr
            );
        goto Exit;
    }

    

Exit:

    if (FAILED(hr))
    {

        if ( pFileTarget )
        {
            pFileTarget->DeleteWdfObject();
        }

        if (pConnection != NULL)
        {
            delete pConnection;            
            pConnection = NULL;
        }
        
        if (pContext != NULL)
        {
            delete pContext;            
            pContext = NULL;
        }

    }
    
    pWdfRequest->Complete(hr);

}


STDMETHODIMP_ (void)
CMyQueue::OnWrite(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ SIZE_T BytesToWrite
    )
/*++

Routine Description:

    Write callback from the framework for this default parallel queue 
    
    The write request needs to be sent to the file handle i/o target associated with this fileobject

Aruments:
    
    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    BytesToWrite - Lenth of bytes in the write buffer

 Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(pWdfQueue);
    UNREFERENCED_PARAMETER(BytesToWrite);    

    //  Call helper function to send request to i/o target 

    SendRequestToFileTarget(pWdfRequest);

    Trace(
        TRACE_LEVEL_INFORMATION, 
        "%!FUNC!"
        );
    
    return;
}

STDMETHODIMP_ (void)
CMyQueue::OnRead(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ SIZE_T BytesToRead
    )
/*++

Routine Description:

   Read callback from the framework for this default parallel queue 

    The read request needs to be sent to the file handle i/o target associated with this fileobject

Aruments:
    
    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    BytesToRead - Lenth of bytes in the read buffer

   
Return Value:

    VOID

--*/
{
     Trace(
        TRACE_LEVEL_INFORMATION, 
        "%!FUNC!"
        );

    UNREFERENCED_PARAMETER(pWdfQueue);
    UNREFERENCED_PARAMETER(BytesToRead);

    //
    // Call helper function to send request to i/o target 
    //
  
    SendRequestToFileTarget(pWdfRequest);

     return;
}

STDMETHODIMP_(void) 
CMyQueue::OnCompletion(
    _In_ IWDFIoRequest* pWdfRequest,
    _In_ IWDFIoTarget* pTarget,
    _In_ IWDFRequestCompletionParams* pCompletionParams,
    _In_ void* pContext
)
/*++

Routine Description:

  This routine is invoked when the request is completed by the lower stack location, 
  in this case the win32 i/o target associated with the file object of this request


   Arguments:
    
   pWdfRequest - wdf request 
   pTarget  - wdf target to which request was earlier sent 
   pCompletionParams  - wdf request completion parameters
   pContext - Context information , if any 
   

Return Value:

    None
--*/    
{
    Trace(
        TRACE_LEVEL_INFORMATION, 
        "%!FUNC!"
        );
    
    UNREFERENCED_PARAMETER(pTarget);
    UNREFERENCED_PARAMETER(pContext);

    // Complete request from the driver 
    pWdfRequest->CompleteWithInformation(
        pCompletionParams->GetCompletionStatus(),
        pCompletionParams->GetInformation());
}

VOID
CMyQueue::SendRequestToFileTarget(
    _In_ IWDFIoRequest* pWdfRequest
)
/*++

Routine Description:
 
  This is a helper functiom to send R/W requests to the win32 file i/o target
  associated with the socket connection for this request. 
  First, filecontext is retrieved which has the file i/o target where this request needs to be sent.
  
       
Arguments:
    
   pWdfRequest - wdf request 

Return Value:

    None

--*/   
{
    
    HRESULT hr; 

    FileContext *pContext = NULL; 
    CComPtr<IWDFFile> pWdfFile = NULL; 

    Trace(
        TRACE_LEVEL_INFORMATION, 
        "%!FUNC!"
        );
    
    //
    // Get the file object for this request 
    //

    pWdfRequest->GetFileObject(&pWdfFile);
    
    //
    // Retrieve Context from file object
    //
    
    hr = pWdfFile->RetrieveContext((void**)&pContext);

    if (pContext == NULL) 
    {
        if ( SUCCEEDED(hr) ) 
        {
            hr = E_FAIL;
            Trace(TRACE_LEVEL_ERROR, 
                  " No Context associated with this file object %!hresult!",
                  hr);
         }
         goto Exit;
    }
    
    //
    // If that succeeds, set completion callback for the request
    //
    pWdfRequest->SetCompletionCallback(CComQIPtr<IRequestCallbackRequestCompletion>(this),
                                       NULL);
   
     //
     // Do not modify the request, format using current type 
     //
     
    pWdfRequest->FormatUsingCurrentType();    

    //
    // Send the request to the win32 i/o target . This was created in OnCreateFile
    //

    hr = pWdfRequest->Send(pContext->pFileTarget,
                           0,
                           0);
Exit:
    
    if (FAILED(hr))
    {
        Trace(TRACE_LEVEL_ERROR, 
              "Could not send request to i/o target %!hresult!",
              hr);
        pWdfRequest->Complete(hr);
    }

    return ;
}

STDMETHODIMP_(void)
CMyQueue::OnCleanup(
    _In_ IWDFObject* /*pWdfObject*/
    )
{
    //
    // CMyQueue has a reference to framework device object via m_FxQueue. 
    // Framework queue object has a reference to CMyQueue object via the callbacks. 
    // This leads to circular reference and both the objects can't be destroyed until this circular reference is broken. 
    // To break the circular reference we release the reference to the framework queue object here in OnCleanup.
    //
    m_FxQueue = NULL;
}
