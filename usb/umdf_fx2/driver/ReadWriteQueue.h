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


#define MAX_TRANSFER_SIZE(x)   64*1024*1024

//
// Queue Callback Object.
//

class CMyReadWriteQueue : 
    public IQueueCallbackRead,
    public IQueueCallbackWrite,
    public IRequestCallbackRequestCompletion,
    public IQueueCallbackIoStop,
    public CMyQueue
{
protected:    
    HRESULT
    Initialize(
        );

    void
    ForwardFormattedRequest(
        _In_ IWDFIoRequest*                         pRequest,
        _In_ IWDFIoTarget*                          pIoTarget
        );
    
public:

    CMyReadWriteQueue(
        _In_ PCMyDevice Device
        );

    virtual ~CMyReadWriteQueue();

    static 
    HRESULT 
    CreateInstance( 
        _In_ PCMyDevice Device,
        _Out_ PCMyReadWriteQueue *Queue
        );

    HRESULT
    Configure(
        VOID
        )
    {
        return CMyQueue::Configure();
    }

    IQueueCallbackWrite *
    QueryIQueueCallbackWrite(
        VOID
        )
    {
        AddRef();
        return static_cast<IQueueCallbackWrite *>(this);
    }

    IQueueCallbackRead *
    QueryIQueueCallbackRead(
        VOID
        )
    {
        AddRef();
        return static_cast<IQueueCallbackRead *>(this);
    }

    IRequestCallbackRequestCompletion *
    QueryIRequestCallbackRequestCompletion(
        VOID
        )
    {
        AddRef();
        return static_cast<IRequestCallbackRequestCompletion *>(this);
    }

    IQueueCallbackIoStop*
    QueryIQueueCallbackIoStop(
        VOID
        )
    {
        AddRef();
        return static_cast<IQueueCallbackIoStop *>(this);
    }
        
    //
    // IUnknown
    //

    STDMETHOD_(ULONG,AddRef) (VOID) {return CUnknown::AddRef();}

    _At_(this, __drv_freesMem(object))
    STDMETHOD_(ULONG,Release) (VOID) {return CUnknown::Release();}

    STDMETHOD_(HRESULT, QueryInterface)(
        _In_ REFIID InterfaceId, 
        _Outptr_ PVOID *Object
        );


    //
    // Wdf Callbacks
    //
    
    //
    // IQueueCallbackWrite
    //
    STDMETHOD_ (void, OnWrite)(
        _In_ IWDFIoQueue *pWdfQueue,
        _In_ IWDFIoRequest *pWdfRequest,
        _In_ SIZE_T NumOfBytesToWrite
        );

    //
    // IQueueCallbackRead
    //
    STDMETHOD_ (void, OnRead)(
        _In_ IWDFIoQueue *pWdfQueue,
        _In_ IWDFIoRequest *pWdfRequest,
        _In_ SIZE_T NumOfBytesToRead
        );

    //
    //IRequestCallbackRequestCompletion
    //
    
    STDMETHOD_ (void, OnCompletion)(
        _In_ IWDFIoRequest*                 pWdfRequest,
        _In_ IWDFIoTarget*                  pIoTarget,
        _In_ IWDFRequestCompletionParams*   pParams,
        _In_ PVOID                          pContext
        );

    //
    //IQueueCallbackIoStop
    //

    STDMETHOD_ (void, OnIoStop)(
        _In_ IWDFIoQueue *   pWdfQueue,
        _In_ IWDFIoRequest * pWdfRequest,
        _In_ ULONG           ActionFlags
        );
    
};
