/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    queue.h

Abstract:

    This file defines the queue callback interface.

Environment:

    user mode only

Revision History:

--*/

#pragma once

//
// Queue Callback Object.
//

class ATL_NO_VTABLE CMyQueue : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IQueueCallbackCreate,
    public IQueueCallbackRead, 
    public IQueueCallbackWrite,
    public IRequestCallbackRequestCompletion,
    public IObjectCleanup    
{
public:

DECLARE_NOT_AGGREGATABLE(CMyQueue)

BEGIN_COM_MAP(CMyQueue)
    COM_INTERFACE_ENTRY(IQueueCallbackCreate)
    COM_INTERFACE_ENTRY(IQueueCallbackRead)
    COM_INTERFACE_ENTRY(IQueueCallbackWrite)
    COM_INTERFACE_ENTRY(IRequestCallbackRequestCompletion)
    COM_INTERFACE_ENTRY(IObjectCleanup)
END_COM_MAP()

public:
    //IQueueCallbackRead
    STDMETHOD_(void,OnRead)(_In_ IWDFIoQueue* pWdfQueue,_In_ IWDFIoRequest* pWdfRequest,_In_ SIZE_T NumOfBytesToRead);

    //IQueueCallbackWrite
    STDMETHOD_(void,OnWrite)(_In_ IWDFIoQueue* pWdfQueue,_In_ IWDFIoRequest* pWdfRequest,_In_ SIZE_T NumOfBytesToWrite);

    //IQueueCallbackCreate
    STDMETHOD_(void,OnCreateFile)(_In_ IWDFIoQueue* pWdfQueue,_In_ IWDFIoRequest* pWDFRequest,_In_ IWDFFile* pWdfFileObject);

   // IRequestCallbackRequestCompletion
   STDMETHOD_(void,OnCompletion)(_In_ IWDFIoRequest* pWdfRequest,_In_ IWDFIoTarget* pTarget,_In_ IWDFRequestCompletionParams* pCompletionParams,_In_ void* pContext);

    //IObjectCleanup
    STDMETHOD_(void,OnCleanup)(_In_ IWDFObject* pWdfObject);

public:
    CMyQueue();
    ~CMyQueue();

    STDMETHOD(Initialize)(_In_ CMyDevice * Device);

    HRESULT
    Configure(
        );
    
private:
    CComPtr<IWDFIoQueue> m_FxQueue;

    //
    // Unreferenced pointer to the parent device.
    //

    CMyDevice * m_Device;

    VOID SendRequestToFileTarget( _In_  IWDFIoRequest* pWdfRequest);    
};
