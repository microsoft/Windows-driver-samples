#pragma once

#include "resource.h"
#include "WpdBasicHardwareDriver.h"

class ATL_NO_VTABLE CDevice :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IPnpCallback,
    public IPnpCallbackSelfManagedIo,
    public IPnpCallbackHardware
{
public:
    CDevice() :
        m_pWpdBaseDriver(NULL)
    {
    }

    DECLARE_NOT_AGGREGATABLE(CDevice)

    BEGIN_COM_MAP(CDevice)
        COM_INTERFACE_ENTRY(IPnpCallback)
        COM_INTERFACE_ENTRY(IPnpCallbackSelfManagedIo)
        COM_INTERFACE_ENTRY(IPnpCallbackHardware)
    END_COM_MAP()

public:
    static HRESULT
    CreateInstance(
        _In_            IWDFDeviceInitialize*   pDeviceInit,
        _In_            WpdBaseDriver*          pWpdBaseDriver,
        _COM_Outptr_    IUnknown**              ppUnkwn)
    {
        *ppUnkwn = NULL;

        //
        // Set device properties.
        //
        pDeviceInit->SetLockingConstraint(None);

        CComObject< CDevice> *pMyDevice = NULL;
        HRESULT hr = CComObject<CDevice>::CreateInstance( &pMyDevice );
        if( SUCCEEDED (hr) )
        {
            pMyDevice->AddRef();
            hr = pMyDevice->QueryInterface( __uuidof(IUnknown),(void **) ppUnkwn);
            if (hr == S_OK)
            {
                pMyDevice->m_pWpdBaseDriver = pWpdBaseDriver;
            }
            pMyDevice->Release();
            pMyDevice = NULL;
        }

        return hr;
    }

    // IPnpCallback
    //
    STDMETHOD_(HRESULT, OnD0Entry)        (_In_ IWDFDevice* pDevice, WDF_POWER_DEVICE_STATE previousState);
    STDMETHOD_(HRESULT, OnD0Exit)         (_In_ IWDFDevice* pDevice, WDF_POWER_DEVICE_STATE newState);
    STDMETHOD_(VOID,     OnSurpriseRemoval)(_In_ IWDFDevice* pDevice);
    STDMETHOD_(HRESULT, OnQueryRemove)    (_In_ IWDFDevice* pDevice);
    STDMETHOD_(HRESULT, OnQueryStop)      (_In_ IWDFDevice* pDevice);

    // IPnpCallbackSelfManagedIo
    //
    STDMETHOD_(VOID,     OnSelfManagedIoCleanup)(_In_ IWDFDevice* pDevice);
    STDMETHOD_(VOID,     OnSelfManagedIoFlush)  (_In_ IWDFDevice* pDevice);
    STDMETHOD_(HRESULT, OnSelfManagedIoInit)   (_In_ IWDFDevice* pDevice);
    STDMETHOD_(HRESULT, OnSelfManagedIoSuspend)(_In_ IWDFDevice* pDevice);
    STDMETHOD_(HRESULT, OnSelfManagedIoRestart)(_In_ IWDFDevice* pDevice);
    STDMETHOD_(HRESULT, OnSelfManagedIoStop)   (_In_ IWDFDevice* pDevice);

    // IPnpCallbackHardware
    //
    STDMETHOD_(HRESULT, OnPrepareHardware)(_In_ IWDFDevice* pDevice);
    STDMETHOD_(HRESULT, OnReleaseHardware)(_In_ IWDFDevice* pDevice);

private:

    HRESULT GetDeviceFriendlyName(
        _Outptr_result_maybenull_ LPWSTR*   pwszDeviceFriendlyName);

private:

    WpdBaseDriver*                         m_pWpdBaseDriver;
    CComPtr<IPortableDeviceClassExtension> m_pPortableDeviceClassExtension;
};

