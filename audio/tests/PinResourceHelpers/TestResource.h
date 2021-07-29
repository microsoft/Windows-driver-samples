// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft. All rights reserved.
//
// File Name:
//
//  TestResource.h
//
// Abstract:
//
//  TAEF Test Resource
//
// -------------------------------------------------------------------------------
#pragma once

#include "HalfApp.h"

class CPinTestResource :
    public WEX::TestExecution::ITestResource,
    public IHalfAppContainer
{
public:
    //IUnknown
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(
        __in REFIID riid,
        __deref_out VOID **ppvObject
    );
    // ITestResouce
    STDMETHODIMP GetGuid(GUID* pGuid);
    STDMETHODIMP SetGuid(GUID guid);
    STDMETHODIMP GetValue(BSTR name, BSTR* pValue);
    STDMETHODIMP SetValue(BSTR name, BSTR value);

    //IHalfAppContainer
    STDMETHODIMP GetHalfApp(CHalfApp ** ppHalfApp);

    static HRESULT STDMETHODCALLTYPE CreateInstance(
        CHalfApp * pHalf,
        REFGUID guid,
        WEX::TestExecution::ITestResource ** ppOut
    );

private:

    CPinTestResource();
    ~CPinTestResource();

    HRESULT STDMETHODCALLTYPE Initialize(
        CHalfApp * pHalf,
        REFGUID guid
    );

    LPWSTR ModeName(REFGUID guidMode);
    LPWSTR PinName(EndpointConnectorType eConnectorType, bool IsMVA);

    CComBSTR m_szType;
    CComBSTR m_szName;
    CComBSTR m_szId;
    CComBSTR m_szMode;
    CComBSTR m_szPin;

    CAutoPtr<CHalfApp> m_spHalfApp;

    ULONG m_cRef;
    GUID m_guid;
};
