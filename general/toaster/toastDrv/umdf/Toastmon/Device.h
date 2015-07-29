/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Device.h

Abstract:

    This module contains the type definitions for the UMDF sample
    driver's device callback class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once
#include "initguid.h"


//{781EF630-72B2-11d2-B852-00C04FAD5171}
DEFINE_GUID(GUID_DEVINTERFACE_TOASTER, 0x781EF630, 0x72B2, 0x11d2, 0xB8, 0x52, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);


//
// Class for the sample device.
//

class CMyDevice : public CUnknown,
                  public IPnpCallbackRemoteInterfaceNotification
{
    //
    // Private data members.
    //
private:

    //
    // Weak reference to the WDF device object.
    //

    IWDFDevice2 *m_FxDevice;

    //
    // Weak reference to the WDF driver object
    //
    
    IWDFDriver  *m_FxDriver;

    //
    // Head of remote target list
    //
    
    LIST_ENTRY m_MyRemoteTargets;

    //
    // Private methods.
    //
private:

    //
    // Protected methods
    //
protected:
    CMyDevice(
        VOID
        ) :
        m_FxDevice(NULL),
        m_FxDriver(NULL)
    {
        InitializeListHead(&m_MyRemoteTargets);
    }
    
    HRESULT
    Initialize(
        _In_ IWDFDriver           * FxDriver,
        _In_ IWDFDeviceInitialize * FxDeviceInit
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
        _In_  IWDFDriver           * FxDriver,
        _In_  IWDFDeviceInitialize * FxDeviceInit,
        _Out_ PCMyDevice           * MyDevice
        );
        
    HRESULT
    Configure(
        VOID
        );
    
    ~CMyDevice(
        VOID
        )
    {
        ATLASSERT(IsListEmpty(&m_MyRemoteTargets));
    }

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
    // IPnpCallbackRemoteInterfaceNotification
    //
    void 
    STDMETHODCALLTYPE
    OnRemoteInterfaceArrival(
        _In_ IWDFRemoteInterfaceInitialize * FxRemoteInterfaceInit
        );
    IPnpCallbackRemoteInterfaceNotification *
    QueryIPnpCallbackRemoteInterfaceNotification(
        VOID
        )
    {
        AddRef();
        return static_cast<IPnpCallbackRemoteInterfaceNotification*>(this);
    }

};
