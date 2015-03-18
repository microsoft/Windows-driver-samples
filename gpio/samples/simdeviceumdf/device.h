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

#define MAX_CONNECTIONS (4)
#define MAX_RESOURCES (10)


//
// The device extension for the device object
//

typedef struct _DEVICE_EXTENSION {

    CSimdevice*              MyDevice;

    IWDFDevice*             Device;
    IWDFDevice3*            Device3;

    IWDFCmResourceList*     CmResourceList;

    LARGE_INTEGER ConnectionId[MAX_CONNECTIONS];

}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Class for the simdevice driver.
//

class CSimdevice :
    public CUnknown,
    public IPnpCallbackSelfManagedIo,
    public IPnpCallbackHardware2,
    public IPnpCallback
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
    CSimdeviceQueue   *m_Queue;

    //
    // device data
    //
    DEVICE_EXTENSION m_DevExtension;

//
// Private methods.
//

private:

    CSimdevice(
        VOID
        )
    {
        m_FxDevice = NULL;
        ZeroMemory(&m_DevExtension, sizeof(DEVICE_EXTENSION));
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

   IPnpCallback *
    QueryIPnpCallback(
        VOID
        )
    {
        AddRef();
        return static_cast<IPnpCallback *>(this);
    }

    IPnpCallbackHardware2 *
    QueryIPnpCallbackHardware2(
        VOID
        )
    {
        AddRef();
        return static_cast<IPnpCallbackHardware2 *>(this);
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
        _Out_ PCSimdevice *Device
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

    __drv_arg(this, __drv_freesMem(object))
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

    //
    // IPnpCallback
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnD0Entry(
        _In_ IWDFDevice*  pWdfDevice,
        _In_ WDF_POWER_DEVICE_STATE  previousState
    );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnD0Exit(
        _In_ IWDFDevice*  pWdfDevice,
        _In_ WDF_POWER_DEVICE_STATE  previousState
    );

    virtual
    void
    STDMETHODCALLTYPE
    OnSurpriseRemoval(
        _In_ IWDFDevice*  pWdfDevice
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnQueryRemove(
        _In_ IWDFDevice*  pWdfDevice
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnQueryStop(
        _In_ IWDFDevice*  pWdfDevice
        );

    //
    // IPnpCallbackHardware2
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnPrepareHardware(
        _In_ IWDFDevice3 * pWdfDevice,
        _In_ IWDFCmResourceList * pWdfResourcesRaw,
        _In_ IWDFCmResourceList * pWdfResourcesTranslated
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnReleaseHardware(
        _In_ IWDFDevice3 * pWdfDevice,
        _In_ IWDFCmResourceList * pWdfResourcesTranslated
        );

    HRESULT 
    SimdeviceConnectInterrupt(
        _In_     IWDFDevice* pWdfDevice,
        _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR RawResource,
        _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedResource
        );

    static WUDF_INTERRUPT_ISR      OnInterruptIsr;
    static WUDF_INTERRUPT_WORKITEM OnInterruptWorkItem;

    IWDFDevice *
    GetFxDevice(
        VOID
        )
    {
        return m_FxDevice;
    }

   PDEVICE_EXTENSION
   GetDeviceExtension(
       VOID
       )
   {
       return &m_DevExtension;
   }

    HRESULT
    TestReadWrite(
        _In_ IWDFDevice* pWdfDevice,
        _In_ PWSTR RequestString,
        _In_ BOOLEAN ReadOperation,
        _Inout_updates_bytes_(Size) UCHAR *Data,
        _In_ ULONG Size,
        _Inout_opt_ IWDFRemoteTarget *IoTargetOut
    );

};

