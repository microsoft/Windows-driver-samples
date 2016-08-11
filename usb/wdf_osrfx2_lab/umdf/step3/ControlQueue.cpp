/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    ControlQueue.cpp

Abstract:

    This file implements the I/O queue interface and performs
    the ioctl operations.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"

#include "winioctl.h"

#include "ControlQueue.tmh"

CMyControlQueue::CMyControlQueue(
    _In_ PCMyDevice Device
    ) : CMyQueue(Device)
{

}

HRESULT
STDMETHODCALLTYPE
CMyControlQueue::QueryInterface(
    _In_ REFIID InterfaceId,
    _Out_ PVOID *Object
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


    if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackDeviceIoControl))) 
    {
        hr = S_OK;
        *Object = QueryIQueueCallbackDeviceIoControl(); 

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
CMyControlQueue::CreateInstance(
    _In_ PCMyDevice Device,
    _Out_ PCMyControlQueue *Queue
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
    PCMyControlQueue queue = NULL;
    HRESULT hr = S_OK;

    queue = new CMyControlQueue(Device);

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
CMyControlQueue::Initialize(
    VOID
    )
{
    HRESULT hr;

    //
    // First initialize the base class.  This will create the partner FxIoQueue
    // object and setup automatic forwarding of I/O controls.
    //
    
    hr = __super::Initialize(WdfIoQueueDispatchSequential, 
                             false, 
                             true);

    //
    // return the status.
    //

    return hr;
}

VOID
STDMETHODCALLTYPE
CMyControlQueue::OnDeviceIoControl(
    _In_ IWDFIoQueue *FxQueue,
    _In_ IWDFIoRequest *FxRequest,
    _In_ ULONG ControlCode,
    _In_ SIZE_T InputBufferSizeInBytes,
    _In_ SIZE_T OutputBufferSizeInBytes
    )
/*++

Routine Description:


    DeviceIoControl dispatch routine

Aruments:
    
    FxQueue - Framework Queue instance
    FxRequest - Framework Request  instance
    ControlCode - IO Control Code
    InputBufferSizeInBytes - Lenth of input buffer
    OutputBufferSizeInBytes - Lenth of output buffer

    Always succeeds DeviceIoIoctl
Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(FxQueue);
    UNREFERENCED_PARAMETER(OutputBufferSizeInBytes);    
    
    IWDFMemory *memory = NULL;
    PVOID buffer;

    SIZE_T bigBufferCb;

    ULONG information = 0;

    bool completeRequest = true;

    HRESULT hr = S_OK;

    switch (ControlCode)
    {
        case IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY:
        {
            //
            // Make sure the buffer is big enough to hold the input for the
            // control transfer.
            //

            if (InputBufferSizeInBytes < sizeof(BAR_GRAPH_STATE))
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                FxRequest->GetInputMemory(&memory);
            }

            //
            // Get the data buffer and use it to set the bar graph on the
            // device.
            //

            if (SUCCEEDED(hr)) 
            {
                buffer = memory->GetDataBuffer(&bigBufferCb);
                memory->Release();

                hr = m_Device->SetBarGraphDisplay((PBAR_GRAPH_STATE) buffer);
            }

            break;
        }

        default:
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
            break;
        }
    }

    if (completeRequest)
    {
        FxRequest->CompleteWithInformation(hr, information);
    }

    return;
}
