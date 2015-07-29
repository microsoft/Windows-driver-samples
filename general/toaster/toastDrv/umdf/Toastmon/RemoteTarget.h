/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    RemoteTarget.h

Abstract:

    This module contains the type definitions for the UMDF sample
    driver's remote target callback class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once


#define READ_BUF_SIZE           100
#define WRITE_BUF_SIZE          120


class CMyRemoteTarget : public CUnknown,
                        public IRequestCallbackRequestCompletion,
                        public IRemoteTargetCallbackRemoval,
                        public IObjectCleanup
{
    //
    // Public data members.
    //
public:

    //
    // This is the Entry for the list of Remote Targets.
    // The List head is held by CMyDevice.
    //
    LIST_ENTRY m_Entry;

    
    //
    // Private data members.
    //
private:

    CMyDevice * m_MyDevice; // Weak reference

    //
    // The handle for a thread to post I/O requests to the remote target
    //
    HANDLE m_hThread;
    
    CComPtr<IWDFRemoteTarget>    m_FxTarget;
    
    bool                         m_WriteInProgress;
    CComPtr<IWDFIoRequest2>      m_FxWriteRequest;
    CComPtr<IWDFMemory>          m_FxWriteMemory;

    bool                         m_ReadInProgress;
    CComPtr<IWDFIoRequest2>      m_FxReadRequest;
    CComPtr<IWDFMemory>          m_FxReadMemory;

    //
    // Private methods.
    //
private:

    void
    PostIoRequests(
        VOID
        );

    //
    // Protected methods
    //
protected:
    
    CMyRemoteTarget(
        VOID
        ) :
        m_MyDevice(NULL),
        m_hThread(NULL),
        m_WriteInProgress(false),
        m_ReadInProgress(false)
        
    {
    }
    
    HRESULT
    Initialize(
        _In_ CMyDevice           * MyDevice,
        _In_ IWDFDriver          * FxDriver,
        _In_ IWDFDevice2         * FxDevice,
        _In_ IWDFRemoteInterface * FxRemoteInterface
        );

    //
    // Public methods
    //
public:

    //
    // The factory method used to create an instance of this driver.
    //
    static
    HRESULT
    CreateInstance(
        _In_ CMyDevice           * MyDevice,
        _In_ IWDFDriver          * FxDriver,
        _In_ IWDFDevice2         * FxDevice,
        _In_ IWDFRemoteInterface * FxRemoteInterface,
        _Out_ PCMyRemoteTarget   * MyRemoteTarget
        );
    
    ~CMyRemoteTarget(
        VOID
        )
    {
    }

    DWORD WINAPI
    ThreadProc(
        VOID
        );

    //
    // COM methods
    //
public:

    //
    // IUnknown methods.
    //
    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        VOID
        )
    {
        return __super::AddRef();
    }
    _At_(this, __drv_freesMem(object))
    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        VOID
       )
    {
        return __super::Release();
    }
    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        _In_  REFIID   InterfaceId,
        _Out_ PVOID  * Object
        );

    //
    // IRequestCallbackRequestCompletion
    //
    void 
    STDMETHODCALLTYPE
    OnCompletion(
        _In_ IWDFIoRequest               * FxRequest,
        _In_ IWDFIoTarget                * /* FxTarget */,
        _In_ IWDFRequestCompletionParams * /* Params */,
        _In_ void*                         /* Context */
        );
    IRequestCallbackRequestCompletion *
    QueryIRequestCallbackRequestCompletion(
        VOID
        )
    {
        AddRef();
        return static_cast<IRequestCallbackRequestCompletion*>(this);
    }

    //
    // IRemoteTargetCallbackRemoval
    //
    BOOL 
    STDMETHODCALLTYPE
    OnRemoteTargetQueryRemove(
        _In_ IWDFRemoteTarget * /* FxTarget */
        );
    VOID 
    STDMETHODCALLTYPE
    OnRemoteTargetRemoveCanceled(
        _In_ IWDFRemoteTarget * /* FxTarget */
        );
    VOID 
    STDMETHODCALLTYPE
    OnRemoteTargetRemoveComplete(
        _In_ IWDFRemoteTarget * /* FxTarget */
        );
    IRemoteTargetCallbackRemoval *
    QueryIRemoteTargetCallbackRemoval(
        VOID
        )
    {
        AddRef();
        return static_cast<IRemoteTargetCallbackRemoval*>(this);
    }

    //
    // IObjectCleanup
    //
    void
    STDMETHODCALLTYPE
    OnCleanup(
        _In_ IWDFObject* /* FxObject */
        );
    IObjectCleanup *
    QueryIObjectCleanup(
        VOID
        )
    {
        AddRef();
        return static_cast<IObjectCleanup*>(this);
    }

};
