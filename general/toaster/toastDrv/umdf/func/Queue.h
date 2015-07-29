/*++

  Copyright (c) Microsoft Corporation, All Rights Reserved

  Module Name:

    Queue.h

  Abstract:

    This file contains the class definition for the queue
    callback object.

  Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once
#include "WUDFToaster.h"

class CQueue :  
    public IQueueCallbackDeviceIoControl,
    public IQueueCallbackRead,
    public IQueueCallbackWrite
{
    public:

        CQueue() : m_cRefs(0)
        {
        }

    public:

        //
        // Static method that creates a queue callback object.
        //        
        static HRESULT CreateInstance(_Out_ IUnknown **ppUkwn)
        {
            *ppUkwn = NULL;
                
#pragma warning( suppress : 6014 )// PFD ISSUE: counted memory locks
            CQueue *pMyQueue = new CQueue();
            
            if (NULL == pMyQueue)
            {
                return E_OUTOFMEMORY;
            }
            return (pMyQueue->QueryInterface(__uuidof(IUnknown), (void **)ppUkwn ));
        }
        
        //
        // IUnknown
        //
        virtual HRESULT __stdcall QueryInterface(_In_ REFIID riid, _Out_ LPVOID* ppvObject);
        virtual ULONG   __stdcall AddRef();
        _At_(this, __drv_freesMem(object))
        virtual ULONG   __stdcall Release();

        //
        // IQueueCallbackDeviceIoControl
        //        
        virtual void __stdcall OnDeviceIoControl(
            _In_ IWDFIoQueue*    pQueue,
            _In_ IWDFIoRequest*  pRequest,
            _In_ ULONG           ControlCode,    
            _In_ SIZE_T          InputBufferSizeInBytes,
            _In_ SIZE_T          OutputBufferSizeInBytes
            );

        //
        // IQueueCallbackRead
        //
        virtual void __stdcall OnRead(
             _In_ IWDFIoQueue *pWdfQueue,
             _In_ IWDFIoRequest *pWdfRequest,
             _In_ SIZE_T NumOfBytesToRead
            );

        //
        // IQueueCallbackWrite
        //
        virtual void __stdcall OnWrite(
            _In_ IWDFIoQueue *pWdfQueue,
            _In_ IWDFIoRequest *pWdfRequest,
            _In_ SIZE_T NumOfBytesToWrite
            );

       //
       // TODO: Add your interfaces here
       //

    private:
        LONG    m_cRefs;
};
