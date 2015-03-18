/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Device.h

Abstract:

    This module contains the type definitions for the UMDF Echo sample
    driver's device callback class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

#include "queue.h"

//
// Class for the iotrace driver.
//

class CMyDevice : 
    public CUnknown,
    public IPnpCallbackSelfManagedIo        
{

//
// Private data members.
//
private:

    IWDFDevice *m_FxDevice;

    //
    // Completion Thread handle used by queue callback object
    //
    HANDLE      m_ThreadHandle;

    //
    // Our queue callback object
    // Strong reference - since we pass it to the thread we create
    //
    CMyQueue   *m_Queue;

//
// Private methods.
//

private:

    CMyDevice(
        VOID
        )
    {
        m_FxDevice = NULL;
    }

    HRESULT
    Initialize(
        _In_ IWDFDriver *FxDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit
        );

    IPnpCallbackSelfManagedIo *
    QueryIPnpCallbackSelfManagedIo(
        VOID
        )
    {
        AddRef();
        return static_cast<IPnpCallbackSelfManagedIo *>(this);
    }
    

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
        _In_ IWDFDriver *FxDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit,
        _Out_ PCMyDevice *Device
        );

    HRESULT
    Configure(
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
        _In_ REFIID InterfaceId,
        _Out_ PVOID *Object
        );

    //
    // IPnpCallbackSelfManagedIo methods
    //

    //
    // We implement this interface to create and tear down
    // our completion thread
    //
    // It is critical that we wait for all the threads we create
    // to exit during OnSelfManagedIoCleanup, otherwise thread
    // may continue to execute when framework unloads the driver,
    // leading to a crash
    //
    // We don't manage any I/O separate from the queue, so apart
    // from OnSelfManagedIoInit and OnSelfManagedIoCleanup, other
    // methods have token implementations
    //
    
    virtual
    void
    STDMETHODCALLTYPE
    OnSelfManagedIoCleanup(
        _In_ IWDFDevice * pWdfDevice
        );
        
    virtual
    void
    STDMETHODCALLTYPE
    OnSelfManagedIoFlush(
        _In_ IWDFDevice * pWdfDevice
        )
    {
        UNREFERENCED_PARAMETER( pWdfDevice );
    }

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnSelfManagedIoInit(
        _In_ IWDFDevice * pWdfDevice
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnSelfManagedIoSuspend(
        _In_ IWDFDevice * pWdfDevice
        )
    { 
        UNREFERENCED_PARAMETER( pWdfDevice );
        
        return S_OK;
    }

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnSelfManagedIoRestart(
        _In_ IWDFDevice * pWdfDevice
        )
    {
        UNREFERENCED_PARAMETER( pWdfDevice );
        
        return S_OK;
    }
    
    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnSelfManagedIoStop(
        _In_ IWDFDevice * pWdfDevice
        )
    {
        UNREFERENCED_PARAMETER( pWdfDevice );
    
        return S_OK;
    }    
};
