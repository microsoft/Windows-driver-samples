#include "stdafx.h"

#include "RS232Target.tmh"

RS232Target::RS232Target() :
    m_cRef(1), m_pBaseDriver(NULL)
{
}

RS232Target::~RS232Target()
{
    Delete();
}

ULONG __stdcall RS232Target::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG __stdcall RS232Target::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return ulRefCount;
}

HRESULT __stdcall RS232Target::QueryInterface(
    REFIID riid,
    void** ppv)
{
    HRESULT hr = S_OK;

    if(riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        AddRef();
    }
    if(riid == __uuidof(IRequestCallbackRequestCompletion))
    {
        *ppv = static_cast<IRequestCallbackRequestCompletion*>(this);
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}



/**
 * This method is called to initialize the RS232 I/O Target
 */
HRESULT RS232Target::Create(_In_ WpdBaseDriver* pBaseDriver,
                            _In_ IWDFDevice*    pDevice, 
                                 HANDLE         hRS232Port)
{
    CComPtr<IWDFFileHandleTargetFactory> pFileHandleTargetFactory;

    HRESULT hr = S_OK;

    if (pDevice == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "NULL parameter received for IWDFDevice");
        return hr;
    }

    m_pWDFDevice = pDevice;
    m_pBaseDriver = pBaseDriver;

    hr = m_pWDFDevice->QueryInterface(IID_PPV_ARGS(&pFileHandleTargetFactory));
    CHECK_HR(hr, "QI of IID_IWDFFileHandleTargetFactory failed");

    if (hr == S_OK)
    {
        hr = pFileHandleTargetFactory->CreateFileHandleTarget(hRS232Port, &m_pFileTarget);
        CHECK_HR(hr, "Failed to create an I/O Target for the port file handle");
    }

    if (hr == S_OK)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, 
                    "%!FUNC! Created win32 I/O target %p for handle %p", m_pFileTarget, hRS232Port);
    }

    return hr;
}

/**
 * This method is called to remove the RS232 I/O Target object
 * and do any cleanup
 */
void RS232Target::Delete()
{
    if (m_pFileTarget)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, 
                    "%!FUNC! Deleted win32 I/O target %p", m_pFileTarget);
        m_pFileTarget = NULL;
    }

    if (m_pWDFDevice)
    {
        m_pWDFDevice = NULL;
    }

    if (m_pBaseDriver)
    {
        m_pBaseDriver = NULL;
    }
}

/**
 * This method is called to start the RS232 I/O Target after it 
 * it has been created
 */
HRESULT RS232Target::Start()
{
    CComPtr<IWDFIoTargetStateManagement> pStateMgmt;

    HRESULT hr                      = S_OK;

    if (m_pFileTarget)
    {
        hr = m_pFileTarget->QueryInterface(IID_PPV_ARGS(&pStateMgmt));
        CHECK_HR(hr, "Failed to QI IWDFIoTargetStateManagement from the I/O target");

        if (hr == S_OK)
        {
            hr = pStateMgmt->Start();
            CHECK_HR(hr, "Failed to start the I/O target");
        }

        if (hr == S_OK && IsReady())
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, "%!FUNC! I/O target ready.  Sending read request");    
            hr = SendReadRequest();  
            CHECK_HR(hr, "SendReadRequest failed");
        }
    }
    return hr;
}

/**
 * This method is called to stop the RS232 I/O Target 
 * if it is currently running
 */
HRESULT RS232Target::Stop()
{
    CComPtr<IWDFIoTargetStateManagement> pStateMgmt;

    HRESULT hr = S_OK;

    // Stop the target only if it is started. 
    if (m_pFileTarget && (WdfIoTargetStarted == GetState()))
    {
        hr = m_pFileTarget->QueryInterface(IID_PPV_ARGS(&pStateMgmt));
        CHECK_HR(hr, "Failed to QI IWDFIoTargetStateManagement from the I/O target");

        if (hr == S_OK)
        {
            // Stop the target, and cancel sent I/O
            hr = pStateMgmt->Stop(WdfIoTargetCancelSentIo);
            CHECK_HR(hr, "Failed to stop the I/O target");
        }

        if (hr == S_OK)
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, "%!FUNC! S_OK");
        }
    }
    return hr;
}

/**
 * This method is called to return the current state of the RS232 I/O Target
 */
WDF_IO_TARGET_STATE RS232Target::GetState()
{
    CComPtr<IWDFIoTargetStateManagement> pStateMgmt;

    WDF_IO_TARGET_STATE State = WdfIoTargetStateUndefined;
    HRESULT             hr    = S_OK;

    if (m_pFileTarget)
    {
        hr = m_pFileTarget->QueryInterface(IID_PPV_ARGS(&pStateMgmt));
        CHECK_HR(hr, "Failed to QI IWDFIoTargetStateManagement from the I/O target");
        
        if (hr == S_OK)
        {
            State = pStateMgmt->GetState();
        }
    }
    return State;
}


/**
 * This method returns TRUE is the target is ready to receive requests
 */
BOOL RS232Target::IsReady()
{
    if (WdfIoTargetStarted == GetState())
    {
        return TRUE;
    }
    return FALSE;
}


/**
 * This method is called to dispatch an asynchronous read request to the RS232 I/O Target
 * with a completion callback
 */
HRESULT RS232Target::SendReadRequest()
{
    CComPtr<IWDFDriver>                        pWdfDriver;
    CComPtr<IWDFFile>                          pWdfFile;
    CComPtr<IWDFMemory>                        pWdfBuffer;
    CComPtr<IWDFIoRequest>                     pWdfReadRequest;
    CComPtr<IRequestCallbackRequestCompletion> pCompletionCallback;

    HRESULT  hr          = S_OK;

    if (m_pWDFDevice == NULL || m_pFileTarget == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
        CHECK_HR(hr, "Device is not ready to receive read requests");
        return hr;
    }

    m_pWDFDevice->GetDriver(&pWdfDriver);

    ZeroMemory((void*)m_pReadBuffer, sizeof(m_pReadBuffer));

    // Create the WDF memory buffer
    hr = pWdfDriver->CreatePreallocatedWdfMemory(m_pReadBuffer, 
                                                 sizeof(m_pReadBuffer), 
                                                 NULL, // no object event callback
                                                 NULL, // driver object as parent
                                                 &pWdfBuffer); 
    CHECK_HR(hr, "Failed to create the pre-allocaed WDF memory");
 
    if (hr == S_OK)
    {
        hr = m_pWDFDevice->CreateRequest(NULL, // no object event callback
                                         m_pWDFDevice, // device object as parent
                                         &pWdfReadRequest);

        CHECK_HR(hr, "Failed to create a WDF request");    
    }

    if (hr == S_OK)
    {
        // Format the read request
        m_pFileTarget->GetTargetFile(&pWdfFile);

        hr = m_pFileTarget->FormatRequestForRead(pWdfReadRequest,
                                                  pWdfFile,
                                                  pWdfBuffer,
                                                  NULL,  // no memory offset
                                                  NULL); // no device offset  

        CHECK_HR(hr, "Failed to format the WDF request for read");    
    }

    if (hr == S_OK)
    {
        this->QueryInterface(IID_PPV_ARGS(&pCompletionCallback));

        // Set the completion callback
        pWdfReadRequest->SetCompletionCallback(pCompletionCallback, NULL);

        hr = pWdfReadRequest->Send(m_pFileTarget, 0, 0);
        CHECK_HR(hr, "Failed to send the read request");
    }

    if (FAILED(hr))
    {
        // Cleanup on failure
        if (pWdfReadRequest)
        {
            pWdfReadRequest->DeleteWdfObject();
        }
    }

    return hr;
}


/**
 * This method is called to dispatch an asynchronous write request to the RS232 I/O Target
 * with a completion callback
 */
HRESULT RS232Target::SendWriteRequest(
    _In_reads_(cbBufferSize) BYTE*           pBuffer,
                             size_t          cbBufferSize)
{
    CComPtr<IWDFDriver>                        pWdfDriver;
    CComPtr<IWDFFile>                          pWdfFile;
    CComPtr<IWDFMemory>                        pWdfBuffer;
    CComPtr<IWDFIoRequest>                     pWdfWriteRequest;
    CComPtr<IRequestCallbackRequestCompletion> pCompletionCallback;

    HRESULT  hr = S_OK;

    if (pBuffer == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "A NULL buffer parameter was received");
        return hr;
    }

    if (m_pWDFDevice == NULL || m_pFileTarget == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
        CHECK_HR(hr, "Device is not ready to receive write requests");
    }

    m_pWDFDevice->GetDriver(&pWdfDriver);
    m_pFileTarget->GetTargetFile(&pWdfFile);

    // Create the WDF memory buffer
    hr = pWdfDriver->CreatePreallocatedWdfMemory(pBuffer, 
                                                 cbBufferSize, 
                                                 NULL, // no object event callback
                                                 NULL, // driver object as parent
                                                 &pWdfBuffer); 
    CHECK_HR(hr, "Failed to create a pre-allocaed Wdf memory buffer from the input buffer of size %d", static_cast<DWORD>(cbBufferSize));
 
    if (hr == S_OK)
    {
        hr = m_pWDFDevice->CreateRequest(NULL, // no object event callback
                                         m_pWDFDevice, // device object as parent
                                         &pWdfWriteRequest);

        CHECK_HR(hr, "Failed to create a WDF request");    
    }

    if (hr == S_OK)
    {
        hr = m_pFileTarget->FormatRequestForWrite(pWdfWriteRequest,
                                                  pWdfFile,
                                                  pWdfBuffer,
                                                  NULL,  // no memory offset
                                                  NULL); // no device offset  

        CHECK_HR(hr, "Failed to format the WDF request for write");    
    }

    if (hr == S_OK)
    {
        this->QueryInterface(IID_PPV_ARGS(&pCompletionCallback));

        // Set the completion callback
        pWdfWriteRequest->SetCompletionCallback(pCompletionCallback, NULL);

        hr = pWdfWriteRequest->Send(m_pFileTarget, 0, 0);
        CHECK_HR(hr, "Failed to send the write request");
    }
    
    if (FAILED(hr))
    {
        // Cleanup on failure
        if (pWdfWriteRequest)
        {
            pWdfWriteRequest->DeleteWdfObject();
        }
    }

    return hr;
}


/**
 * This callback method is called by UMDF on the completion of the asynchronous reads and writes
 */
void RS232Target::OnCompletion(
    _In_ IWDFIoRequest*                 pWdfRequest,
    _In_ IWDFIoTarget*                  pIoTarget,
    _In_ IWDFRequestCompletionParams*   pParams,
    _In_ PVOID                          pContext)
{
    UNREFERENCED_PARAMETER(pIoTarget);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pParams);

    CComPtr<IWDFRequestCompletionParams>   pCompletionParams;
    HRESULT hr = S_OK;

    // Get the request completion status
    pWdfRequest->GetCompletionParams(&pCompletionParams);

    HRESULT hrStatus = pCompletionParams->GetCompletionStatus();
    WDF_REQUEST_TYPE RequestType = pCompletionParams->GetCompletedRequestType();

    if (RequestType == WdfRequestRead)
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_FLAG_DEVICE, "%!FUNC! Read Status %!HRESULT!", hrStatus);

        // Retrieve the data from the completed read request if successful
        if (SUCCEEDED(hrStatus) && m_pBaseDriver)
        {
            m_pBaseDriver->ProcessReadData(m_pReadBuffer, sizeof(m_pReadBuffer));
        }

        // Send another read request if the target is not stopped or removed
        if (IsReady())
        {
            hr = SendReadRequest();  
            CHECK_HR(hr, "SendReadRequest failed");
        }


    } 
    else if (RequestType == WdfRequestWrite)
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_FLAG_DEVICE, "%!FUNC! Write Status %!HRESULT!", hrStatus);
    }

    // Clean up the existing request
    pWdfRequest->DeleteWdfObject();
}
