/*++

  Copyright (c) Microsoft Corporation, All Rights Reserved

  Module Name:

    Queue.cpp

  Abstract:

    This file contains the queue callback object implementation.

  Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "stdafx.h"
#include "Queue.h"
#include <devioctl.h>
#include <public.h>


#include "internal.h"
#include "queue.tmh"

HRESULT
CQueue::QueryInterface(
    _In_ REFIID riid,
    _Out_ LPVOID* ppvObject
    )
/*++

Routine Description:

    The framework calls this function to determine which callback
    interfaces we support.

Arguments:

    riid        - GUID for a given callback interface.
    ppvObject   - We set this pointer to our object if we support the
                  interface indicated by riid.

Return Value:

   HRESULT S_OK - Interface is supported.

--*/
{
    if (ppvObject == NULL)
    {
        return E_INVALIDARG;
    }
    *ppvObject = NULL;

    if ( riid == _uuidof(IUnknown) )
    {
        *ppvObject = static_cast<IQueueCallbackDeviceIoControl *> (this);
    }    
    else if ( riid == _uuidof(IQueueCallbackDeviceIoControl) )
    {
        *ppvObject = static_cast<IQueueCallbackDeviceIoControl *>(this);
    }
    else if ( riid == _uuidof(IQueueCallbackRead) )
    {
        *ppvObject = static_cast<IQueueCallbackRead *>(this);
    }
    else if ( riid == _uuidof(IQueueCallbackWrite) )
    {
        *ppvObject = static_cast<IQueueCallbackWrite *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    this->AddRef();

    return S_OK;
}



ULONG CQueue::AddRef()
/*++

Routine Description:

    Increments the ref count on this object.

Arguments:

    None.

Return Value:

    ULONG - new ref count.

--*/
{
    LONG cRefs = InterlockedIncrement( &m_cRefs );
    return cRefs;
}

_At_(this, __drv_freesMem(object))
ULONG CQueue::Release()
/*++

Routine Description:

    Decrements the ref count on this object.

Arguments:

    None.

Return Value:

    ULONG - new ref count.

--*/
{
    LONG cRefs;

    cRefs = InterlockedDecrement( &m_cRefs );

    if( 0 == cRefs )
    {
        delete this;
    }

    return cRefs;
}


void
CQueue::OnDeviceIoControl(
    _In_ IWDFIoQueue*    pQueue,
    _In_ IWDFIoRequest*  pRequest,
    _In_ ULONG           ControlCode,   
    _In_ SIZE_T         /*InputBufferSizeInBytes*/,
    _In_ SIZE_T         /*OutputBufferSizeInBytes*/        
    )
/*++

Routine Description:

    The framework calls this function when somone has called
    DeviceIoControl on the device.

Arguments:

Return Value:
    None

--*/
{
    HRESULT     hr = S_OK;
    IWDFDevice  *pDevice = NULL;

    Trace(TRACE_LEVEL_INFORMATION,"%!FUNC!");

    //
    // Retrieve the queue's parent device object
    //
    pQueue->GetDevice(&pDevice);
    
    WUDF_TEST_DRIVER_ASSERT(pDevice);
    
    switch (ControlCode)
    {
       case IOCTL_TOASTER_DONT_DISPLAY_IN_UI_DEVICE:
   
           //
           // This is just an example on how to hide your device in the 
           // device manager. Please remove your code when you adapt this 
           // sample for your hardware.
           //
             pDevice->SetPnpState(WdfPnpStateDontDisplayInUI, WdfTrue);
             pDevice->CommitPnpState();
             
             break;
   
       default:
            hr = E_FAIL; //invalid request

            Trace(TRACE_LEVEL_ERROR,"%!FUNC! Invalid IOCTL %!hresult!",hr);
    }
    pRequest->Complete(hr);
    
    return;
}

void
CQueue::OnRead(
    _In_ IWDFIoQueue* /* pQueue */,
    _In_ IWDFIoRequest* pRequest,
    _In_ SIZE_T SizeInBytes 
    )
/*++

Routine Description:


    Read dispatch routine
    IQueueCallbackRead

Arguments:
    
    pQueue - Framework Queue instance
    pRequest - Framework Request  instance
    SizeInBytes - Length of bytes in the read buffer

    Copy available data into the read buffer

Return Value:
    None.

--*/
{      
    Trace(TRACE_LEVEL_INFORMATION,"%!FUNC!");

    //
    // No need to check for zero-length reads.
    //
    // The framework queue is created with the flag bAllowZeroLengthRequests = FALSE.
    // FALSE indicates that the framework completes zero-length I/O requests instead
    // of putting them in the I/O queue. 
    //

    //
    // TODO: Put your Read request processing here
    //

    pRequest->CompleteWithInformation(S_OK, SizeInBytes);

    return;

}

void
CQueue::OnWrite(
    _In_ IWDFIoQueue * /* pQueue */,
    _In_ IWDFIoRequest * pRequest,
    _In_ SIZE_T BytesToWrite 
    )
/*++

Routine Description:

    Write dispatch routine
    IQueueCallbackWrite

Arguments:
    
    pQueue - Framework Queue instance
    pRequest - Framework Request  instance
    BytesToWrite - Length of bytes in the write buffer

    Allocate and copy data to local buffer

Return Value:
    None.    

--*/
{  
    Trace(TRACE_LEVEL_INFORMATION,"%!FUNC!");

    //
    // No need to check for zero-length writes.
    //   
    
    //
    // TODO: Put your Write request processing here
    //

    pRequest->CompleteWithInformation(S_OK, BytesToWrite);

    return;
}

