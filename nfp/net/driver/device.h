/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Device.h

Abstract:

    This module contains the type definitions for the sample
    driver's device callback class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/
#pragma once

class CMyQueue;

class ATL_NO_VTABLE CMyDevice : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IFileCallbackCleanup,
    public IFileCallbackClose,
    public IObjectCleanup
{
public:

DECLARE_NOT_AGGREGATABLE(CMyDevice)

BEGIN_COM_MAP(CMyDevice)
    COM_INTERFACE_ENTRY(IFileCallbackCleanup)
    COM_INTERFACE_ENTRY(IFileCallbackClose)
    COM_INTERFACE_ENTRY(IObjectCleanup)
END_COM_MAP()

public:
    
    //IFileCallbackCleanup
    STDMETHOD_(void,OnCleanupFile)(_In_ IWDFFile* pWdfFileObject);
    //IFileCallbackClose
    STDMETHOD_(void,OnCloseFile)(_In_ IWDFFile* pWdfFileObject);
    //IObjectCleanup
    STDMETHOD_(void,OnCleanup)(_In_ IWDFObject* pWdfObject);

public:

    STDMETHOD(Initialize)(_In_ IWDFDriver* pWdfDriver, _In_ IWDFDeviceInitialize* pWdfDeviceInit);
    
    HRESULT
    Configure(
        VOID
        );

    IWDFDevice *
    GetFxDevice(
        VOID
        )
    {
        return m_FxDevice;
    }

private:
    CComPtr<IWDFDevice> m_FxDevice;

    CMyQueue* m_MyQueue;
    
    HRESULT  ReadAndAssignPropertyStoreValue();
    HRESULT GetAnsiValFromPropVariant(_In_ PROPVARIANT val, _Inout_ LPSTR *PropertyValueA);
    
};
