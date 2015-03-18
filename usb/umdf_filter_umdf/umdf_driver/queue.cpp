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
#include "queue.tmh"

CMyQueue::CMyQueue(
    _In_ PCMyDevice Device
    ) : 
    m_FxQueue(NULL),
    m_Device(Device)
{
}

//
// Queue destructor.
// Free up the buffer, wait for thread to terminate and 
//

CMyQueue::~CMyQueue(
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
CMyQueue::QueryInterface(
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

    hr = CUnknown::QueryInterface(InterfaceId, Object);

    return hr;
}

//
// Initialize 
//

HRESULT
CMyQueue::Initialize(
    _In_ WDF_IO_QUEUE_DISPATCH_TYPE DispatchType,
    _In_ bool Default,
    _In_ bool PowerManaged
    )
{
    IWDFIoQueue *fxQueue;
    HRESULT hr;

    //
    // Create the I/O Queue object.
    //

    {
        IUnknown *callback = QueryIUnknown();

        hr = m_Device->GetFxDevice()->CreateIoQueue(
                                        callback,
                                        Default,
                                        DispatchType,
                                        PowerManaged,
                                        FALSE,
                                        &fxQueue
                                        );
        callback->Release();
    }

    if (SUCCEEDED(hr))
    {
        m_FxQueue = fxQueue;

        //
        // Release the creation reference on the queue.  This object will be 
        // destroyed before the queue so we don't need to have a reference out 
        // on it.
        //

        fxQueue->Release();
    }

    return hr;
}

HRESULT
CMyQueue::Configure(
    VOID
    )
{
    return S_OK;
}
