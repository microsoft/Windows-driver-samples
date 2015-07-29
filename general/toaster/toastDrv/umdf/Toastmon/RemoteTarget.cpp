/*++
 
Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    RemoteTarget.cpp

Abstract:

    This module contains the implementation of the UMDF sample driver's
    remote interface and remote target callback object.

    This sample demonstrates how to open the remote target and register 
    and respond to device change notification.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "remotetarget.tmh"


DWORD WINAPI _ThreadProc(
    _In_  LPVOID lpParameter
    )
{
    CMyRemoteTarget *MyRemoteTarget = (CMyRemoteTarget*)lpParameter;

    return MyRemoteTarget->ThreadProc();
}

HRESULT
CMyRemoteTarget::CreateInstance(
    _In_ CMyDevice           * MyDevice,
    _In_ IWDFDriver          * FxDriver,
    _In_ IWDFDevice2         * FxDevice,
    _In_ IWDFRemoteInterface * FxRemoteInterface,
    _Out_ PCMyRemoteTarget   * MyRemoteTarget
    )
/*++

  Routine Description:

    This method creates and initializs an instance of the driver's
    remote target callback object.

  Arguments:

    FxRemoteInterfaceInit - An identifier that specifies the remote toaster device.

    MyDevice - a location to store the referenced pointer to the device object.

  Return Value:

    Status

--*/
{
    PCMyRemoteTarget myRemoteTarget;
    HRESULT hr;

    //
    // Allocate a new instance of the remote target class.
    //

    myRemoteTarget = new CMyRemoteTarget();

    if (NULL == myRemoteTarget)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize the instance.
    //

    hr = myRemoteTarget->Initialize(MyDevice, 
                                    FxDriver, 
                                    FxDevice, 
                                    FxRemoteInterface);

    if (SUCCEEDED(hr))
    {
        *MyRemoteTarget = myRemoteTarget;
    }
    else
    {
        myRemoteTarget->Release();
    }

    return hr;
}

HRESULT
CMyRemoteTarget::Initialize(
    _In_ CMyDevice           * MyDevice,
    _In_ IWDFDriver          * FxDriver,
    _In_ IWDFDevice2         * FxDevice,
    _In_ IWDFRemoteInterface * FxRemoteInterface
    )
/*++
 
  Routine Description:

    This method initializes the remote target callback object and creates 
    the partner remote target object.

  Arguments:

    FxRemoteInterface - the identifier for the remote toaster device.

  Return Value:

    status.

--*/
{
    HRESULT hr;
    
    CComPtr<IWDFIoRequest> fxWriteRequest;
    CComPtr<IWDFIoRequest> fxReadRequest;

    //
    // Save a weak reference to the class that created us
    // so we can notify it when we get removed
    //
    
    m_MyDevice = MyDevice;

    //
    // QueryIUnknown references the IUnknown interface that it returns
    // (which is the same as referencing the CMyRemoteTarget).  We pass that
    // to the various Create* calls, which take their own reference if 
    // everything works.
    //

    IUnknown * unknown = this->QueryIUnknown();


    //
    // Create a new FX remote target object and assign the new callback 
    // object to handle any remote target level events that occur.
    //
    hr = FxDevice->CreateRemoteTarget(unknown,
                                      FxRemoteInterface,
                                      &m_FxTarget);

    if (SUCCEEDED(hr))
    {
        //
        // Open and start the remote target. Note that this sample doesn't
        // perform any impersonation, so we're running as "Local Service"
        // since that is what UMDF driver runs as.
        //
        // Make sure that your Toaster devices all have security ACLs that
        // permit "Local Service" to access the device. The Win7 toaster
        // sample driver INFs have been updated for this. However, if you
        // had previously installed pre-Win7 versions of the Toaster sample
        // driver, the security settings will not be updated by a new driver
        // install, unless the Toaster device class key is deleted.
        //

        hr = m_FxTarget->OpenRemoteInterface(FxRemoteInterface, 
                                             NULL,
                                             GENERIC_READ | GENERIC_WRITE,
                                             NULL);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create a new FX request object and assign the new callback 
        // object to handle any request level events that occur.
        //
        hr = FxDevice->CreateRequest(unknown,
                                     FxRemoteInterface,
                                     &fxWriteRequest);
    }

    if (SUCCEEDED(hr))
    {
        //
        // We want to save the newer IWDFIoRequest2 interface instead.
        //
        hr = fxWriteRequest->QueryInterface(IID_PPV_ARGS(&m_FxWriteRequest));
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create a buffer for the write request
        //
        hr = FxDriver->CreateWdfMemory(WRITE_BUF_SIZE, 
                                       NULL, 
                                       FxRemoteInterface, 
                                       &m_FxWriteMemory);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create a new FX remote target object and assign the new callback 
        // object to handle any remote target level events that occur.
        //
        hr = FxDevice->CreateRequest(unknown,
                                     FxRemoteInterface,
                                     &fxReadRequest);
    }

    if (SUCCEEDED(hr))
    {
        //
        // We want to save the newer IWDFIoRequest2 interface instead.
        //
        hr = fxReadRequest->QueryInterface(IID_PPV_ARGS(&m_FxReadRequest));
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create a buffer for the read request
        //
        hr = FxDriver->CreateWdfMemory(READ_BUF_SIZE,
                                       NULL,
                                       FxRemoteInterface,
                                       &m_FxReadMemory);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create/Start the thread which will post I/O requests to the remote
        // target.
        //
        // NOTE: This would not be a typical driver pattern. Normally, your
        //       driver would receive some I/O from another caller and you'd
        //       use this I/O as a trigger to post I/O to the remote target.
        //       You may choose to forward the request directly with no changes,
        //       modify the request before sending, or create an entirely
        //       separate request.
        //
        m_hThread = CreateThread(NULL,
                                 0,
                                 _ThreadProc,
                                 this,
                                 0,
                                 NULL);

        if (m_hThread == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    
    unknown->Release();

    return hr;
}

DWORD WINAPI CMyRemoteTarget::ThreadProc(
    VOID
    )
/*++
 
  Routine Description:

    This method is the main loop for a thread that posts I/O requests to
    the remote target. The main loop continues until the remote target
    is deleted, which happens in the Dispose() method.

--*/
{
    BOOL bContinue = TRUE;

    while(bContinue)
    {
        Sleep(100);
        
        switch(m_FxTarget->GetState())
        {
        case WdfIoTargetStarted:
            PostIoRequests();
            break;
            
        case WdfIoTargetClosedForQueryRemove:
            break;
            
        case WdfIoTargetClosed:
        case WdfIoTargetDeleted:
        default:
            bContinue = false;
            break;
        }
    }

    return 0;
}

void
CMyRemoteTarget::PostIoRequests(
    VOID
    )
{
    HRESULT hr;

    if (!m_WriteInProgress)
    {
        //
        // Mark that the request has been sent, so we don't try to send 
        // again before the completion routine runs.
        //
        m_WriteInProgress = true;

        hr = m_FxTarget->FormatRequestForWrite(m_FxWriteRequest,
                                               NULL,
                                               m_FxWriteMemory,
                                               0,
                                               0);

        if (SUCCEEDED(hr))
        {
            m_FxWriteRequest->SetCompletionCallback(this, NULL);
            
            hr = m_FxWriteRequest->Send(m_FxTarget, 0, 0);
        }
        
        if (FAILED(hr))
        {
            m_WriteInProgress = false;
        }
    }

    if (!m_ReadInProgress)
    {
        //
        // Mark that the request has been sent, so we don't try to send 
        // again before the completion routine runs.
        //
        m_ReadInProgress = true;
        
        hr = m_FxTarget->FormatRequestForRead(m_FxReadRequest,
                                              NULL,
                                              m_FxReadMemory,
                                              0,
                                              0);

        if (SUCCEEDED(hr))
        {
            m_FxReadRequest->SetCompletionCallback(this, NULL);
            
            hr = m_FxReadRequest->Send(m_FxTarget, 0, 0);
        }
        
        if (FAILED(hr))
        {
            m_ReadInProgress = true;
        }
    }
}

HRESULT
CMyRemoteTarget::QueryInterface(
    _In_  REFIID  InterfaceId,
    _Out_ PVOID * Object
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

    if(IsEqualIID(InterfaceId, __uuidof(IRequestCallbackRequestCompletion))) 
    {    
        *Object = QueryIRequestCallbackRequestCompletion();
        hr = S_OK;  
    }
    else if(IsEqualIID(InterfaceId, __uuidof(IRemoteTargetCallbackRemoval))) 
    {    
        *Object = QueryIRemoteTargetCallbackRemoval();
        hr = S_OK;  
    }
    else if(IsEqualIID(InterfaceId, __uuidof(IObjectCleanup))) 
    {    
        *Object = QueryIObjectCleanup();
        hr = S_OK;  
    }
    else
    {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }
    
    return hr;
}

//
// IRequestCallbackRequestCompletion
//
void 
STDMETHODCALLTYPE
CMyRemoteTarget::OnCompletion(
    _In_ IWDFIoRequest               * FxRequest,
    _In_ IWDFIoTarget                * /* FxTarget */,
    _In_ IWDFRequestCompletionParams * /* Params */,
    _In_ void*                         /* Context */
    )
{
    IWDFRequestCompletionParams * CompletionParams;

    FxRequest->GetCompletionParams(&CompletionParams);
    
    if (CompletionParams->GetCompletedRequestType() == WdfRequestRead)
    {
        m_FxReadRequest->Reuse(E_FAIL);
        m_ReadInProgress = false;
    }
    if (CompletionParams->GetCompletedRequestType() == WdfRequestWrite)
    {
        m_FxWriteRequest->Reuse(E_FAIL);
        m_WriteInProgress = false;
    }

    CompletionParams->Release();
}

//
// IRemoteTargetCallbackRemoval
//
BOOL 
STDMETHODCALLTYPE
CMyRemoteTarget::OnRemoteTargetQueryRemove(
    _In_ IWDFRemoteTarget * /* FxTarget */
    )
{
    m_FxTarget->CloseForQueryRemove();

    //
    // Return FALSE if you want to VETO the Query
    //
    
    return TRUE;
}

VOID 
STDMETHODCALLTYPE
CMyRemoteTarget::OnRemoteTargetRemoveCanceled(
    _In_ IWDFRemoteTarget * /* FxTarget */
    )
{
    if (FAILED(m_FxTarget->Reopen()))
    {
        m_FxTarget->Close();
    }
}

VOID 
STDMETHODCALLTYPE
CMyRemoteTarget::OnRemoteTargetRemoveComplete(
    _In_ IWDFRemoteTarget * /* FxTarget */
    )
{
    //
    // The remote device has been removed, so we close the target for good.
    // The rest of cleanup will occur when the framework handles the removal 
    // of the RemoteInterface
    //

    m_FxTarget->Close();
}


//
// IObjectCleanup
//

void
STDMETHODCALLTYPE
CMyRemoteTarget::OnCleanup(
    _In_ IWDFObject * /* FxObject */
    )
{
    if (m_hThread != NULL)
    {
        if (m_FxTarget->GetState() != WdfIoTargetClosed)
        {
            //
            // Close the target if it's still open.
            //
            // If the target has already gone through RemoveComplete, it should 
            // already be closed.
            //
            // If the Target interface is disabled without the device being 
            // removed, the target will still be open until now.
            //
            // If the ToastMon device itself is removed while a target is 
            // still active, we'll now close it.
            //
            m_FxTarget->Close();
        }
        
        //
        // Wait for the Thread to complete
        //
        WaitForSingleObject(m_hThread, INFINITE);

        CloseHandle(m_hThread);

        m_hThread = NULL;
        
        //
        // Remove ourselves from the list of Remote Targets
        //
        RemoveEntryList(&m_Entry);
    }
    
    m_FxTarget = NULL;
    m_FxWriteRequest = NULL;
    m_FxReadRequest = NULL;
}

