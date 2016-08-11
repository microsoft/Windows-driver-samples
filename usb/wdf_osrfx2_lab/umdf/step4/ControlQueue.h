/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    ControlQueue.h

Abstract:

    This file defines the queue callback object for handling device I/O
    control requests.  This is a serialized queue.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

//
// Queue Callback Object.
//

class CMyControlQueue : public IQueueCallbackDeviceIoControl,
                        public CMyQueue
{
    HRESULT
    Initialize(
        VOID
        );

public:

    CMyControlQueue(
        _In_ PCMyDevice Device
        );

    virtual 
    ~CMyControlQueue(
        VOID
        )
    {
        return;
    }

    static 
    HRESULT 
    CreateInstance( 
        _In_ PCMyDevice Device,
        _Out_ PCMyControlQueue *Queue
        );

    HRESULT
    Configure(
        VOID
        )
    {
        return S_OK;
    }
    
    IQueueCallbackDeviceIoControl *
    QueryIQueueCallbackDeviceIoControl(
        VOID
        )
    {
        AddRef();
        return static_cast<IQueueCallbackDeviceIoControl *>(this);
    }

    //
    // IUnknown
    //

    STDMETHOD_(ULONG,AddRef) (VOID) {return CUnknown::AddRef();}

    _At_(this, __drv_freesMem(object))
    STDMETHOD_(ULONG,Release) (VOID) {return CUnknown::Release();}

    STDMETHOD_(HRESULT, QueryInterface)(
        _In_ REFIID InterfaceId, 
        _Out_ PVOID *Object
        );

    //
    // Wdf Callbacks
    //
    
    //
    // IQueueCallbackDeviceIoControl
    //
    STDMETHOD_ (void, OnDeviceIoControl)( 
        _In_ IWDFIoQueue *pWdfQueue,
        _In_ IWDFIoRequest *pWdfRequest,
        _In_ ULONG ControlCode,
        _In_ SIZE_T InputBufferSizeInBytes,
        _In_ SIZE_T OutputBufferSizeInBytes
           );
};

