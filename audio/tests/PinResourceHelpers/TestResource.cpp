// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// File Name:
//
//  TestResource.cpp
//
// Abstract:
//
//  TAEF Test Resource implementation
//
// -------------------------------------------------------------------------------

#include "PreComp.h"
#include <HalfApp.h>
#include <TestResource.h>

using namespace WEX::Logging;
using namespace WEX::TestExecution;

CPinTestResource::CPinTestResource() :
    m_cRef(1)
{}

CPinTestResource::~CPinTestResource()
{}

HRESULT STDMETHODCALLTYPE
CPinTestResource::CreateInstance
(
    CHalfApp * pHalf,
    REFGUID guid,
    ITestResource ** ppOut
)
{
    HRESULT hr = S_OK;
    CPinTestResource * pTestResource;
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    if (!VERIFY_IS_NOT_NULL(pHalf))
    {
        return E_INVALIDARG;
    }

    if (!VERIFY_IS_NOT_NULL(ppOut))
    {
        return E_POINTER;
    }

    pTestResource = new CPinTestResource();
    if (!VERIFY_IS_NOT_NULL(pTestResource))
    {
        return E_OUTOFMEMORY;
    }

    if (!VERIFY_SUCCEEDED(hr = pTestResource->Initialize(
        pHalf, guid))) {
        pTestResource->Release();
        return hr;
    }

    *ppOut = pTestResource;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CPinTestResource::Initialize
(
    CHalfApp * pHalf,
    REFGUID guid
)
{
    HRESULT hr = S_OK;

    WCHAR szStr[MAX_PATH];
    LPWSTR szMode;
    LPWSTR szPin;

    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    m_guid = guid;
    m_spHalfApp.Attach(pHalf);

    // Dataflow
    if (m_spHalfApp->m_DataFlow == capture)
    {
        m_szType.Attach(W2BSTR(L"Capture"));
        if (!VERIFY_IS_NOT_NULL(m_szType)) { return E_OUTOFMEMORY; }
    }
    else
    {
        m_szType.Attach(W2BSTR(L"Render"));
        if (!VERIFY_IS_NOT_NULL(m_szType)) { return E_OUTOFMEMORY; }
    }

    szMode = ModeName(m_spHalfApp->m_Mode);
    szPin = PinName(m_spHalfApp->m_ConnectorType, m_spHalfApp->m_bIsMVA);

    // Id: Combine device Id, pin type and mode
    m_szId.Attach(W2BSTR(m_spHalfApp->m_pwstrDeviceId.get()));
    if (!VERIFY_SUCCEEDED(hr = StringCchPrintfW(
        szStr, MAX_PATH, L"#%s#%s",
        szPin,
        szMode))) {
        return hr;
    }
    if (!VERIFY_SUCCEEDED(hr = m_szId.Append(szStr))) {
        return hr;
    }
    if (!VERIFY_IS_NOT_NULL(m_szId)) { return E_OUTOFMEMORY; }

    // Name: Combine device friendly name, pin type, and mode
    m_szName.Attach(W2BSTR(m_spHalfApp->m_pwstrDeviceFriendlyName.get()));
    if (!VERIFY_SUCCEEDED(hr = StringCchPrintfW(
        szStr, MAX_PATH, L" Pin %s Mode %s",
        szPin,
        szMode))) {
        return hr;
    }
    if (!VERIFY_SUCCEEDED(hr = m_szName.Append(szStr))) {
        return hr;
    }
    if (!VERIFY_IS_NOT_NULL(m_szName)) { return E_OUTOFMEMORY; }

    // Mode
    m_szMode.Attach(W2BSTR(szMode));
    if (!VERIFY_IS_NOT_NULL(m_szMode)) { return E_OUTOFMEMORY; }

    // Pin
    m_szPin.Attach(W2BSTR(szPin));
    if (!VERIFY_IS_NOT_NULL(m_szPin)) { return E_OUTOFMEMORY; }

    return hr;
}

STDMETHODIMP_(ULONG)
CPinTestResource::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CPinTestResource::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

STDMETHODIMP
CPinTestResource::QueryInterface(
    __in REFIID riid,
    __deref_out void ** ppvObject
)
{
    HRESULT hr = S_OK;

    //IF_FALSE_EXIT_HR( (NULL != ppvObject), E_POINTER);

    if (IsEqualIID(riid, __uuidof(IUnknown)))
    {
        *ppvObject = static_cast<void *>(this);
    }
    else if (IsEqualIID(riid, __uuidof(ITestResource)))
    {
        *ppvObject = static_cast<ITestResource *>(this);
    }
    else if (IsEqualIID(riid, __uuidof(IHalfAppContainer)))
    {
        *ppvObject = static_cast<IHalfAppContainer *>(this);
    }
    else
    {
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
        goto Exit;
    }

    AddRef();

Exit:
    return hr;
}

STDMETHODIMP
CPinTestResource::GetGuid(GUID* pGuid) {
    // don't want to see VERIFY spew
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    if (!VERIFY_IS_NOT_NULL(pGuid)) { return E_POINTER; }

    *pGuid = m_guid;
    return S_OK;
}

STDMETHODIMP
CPinTestResource::SetGuid(GUID guid) {
    m_guid = guid;
    return S_OK;
}

STDMETHODIMP
CPinTestResource::GetValue(BSTR name, BSTR* pValue) {
    // don't want to see VERIFY spew
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    if (!VERIFY_IS_NOT_NULL(name)) { return E_POINTER; }
    if (!VERIFY_IS_NOT_NULL(pValue)) { return E_POINTER; }

    if (0 == wcscmp(name, TestResourceProperty::c_szName)) {
        if (!VERIFY_IS_NOT_NULL(*pValue = SysAllocString(m_szName))) { return E_OUTOFMEMORY; }
        return S_OK;
    }

    if (0 == wcscmp(name, TestResourceProperty::c_szId)) {
        if (!VERIFY_IS_NOT_NULL(*pValue = SysAllocString(m_szId))) { return E_OUTOFMEMORY; }
        return S_OK;
    }

    if (0 == wcscmp(name, TestResourceProperty::c_szType)) {
        if (!VERIFY_IS_NOT_NULL(*pValue = SysAllocString(m_szType))) { return E_OUTOFMEMORY; }
        return S_OK;
    }

    if (0 == wcscmp(name, TestResourceProperty::c_szMode)) {
        if (!VERIFY_IS_NOT_NULL(*pValue = SysAllocString(m_szMode))) { return E_OUTOFMEMORY; }
        return S_OK;
    }

    if (0 == wcscmp(name, TestResourceProperty::c_szPin)) {
        if (!VERIFY_IS_NOT_NULL(*pValue = SysAllocString(m_szPin))) { return E_OUTOFMEMORY; }
        return S_OK;
    }

    Log::Error(L"CDevice::GetValue name not found");
    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

STDMETHODIMP
CPinTestResource::SetValue(BSTR name, BSTR value) {
    // don't want to see VERIFY spew
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    if (!VERIFY_IS_NOT_NULL(name)) { return E_POINTER; }
    if (!VERIFY_IS_NOT_NULL(value)) { return E_POINTER; }

    if (0 == wcscmp(name, TestResourceProperty::c_szName)) {
        return m_szName.AssignBSTR(value);
    }

    if (0 == wcscmp(name, TestResourceProperty::c_szId)) {
        return m_szId.AssignBSTR(value);
    }

    if (0 == wcscmp(name, TestResourceProperty::c_szType)) {
        return m_szType.AssignBSTR(value);
    }

    if (0 == wcscmp(name, TestResourceProperty::c_szMode)) {
        return m_szMode.AssignBSTR(value);
    }

    if (0 == wcscmp(name, TestResourceProperty::c_szPin)) {
        return m_szPin.AssignBSTR(value);
    }

    Log::Error(L"CDevice::SetValue name not found");

    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

STDMETHODIMP
CPinTestResource::GetHalfApp(CHalfApp ** ppHalfApp)
{
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    if (!VERIFY_IS_NOT_NULL(ppHalfApp)) { return E_POINTER; }

    *ppHalfApp = m_spHalfApp;

    return S_OK;
}

static const struct
{
    GUID mode;
    LPWSTR name;
}knownModes[] =
{
    { GUID_NULL, L"NO_MODE" },
    { AUDIO_SIGNALPROCESSINGMODE_RAW, L"RAW" },
    { AUDIO_SIGNALPROCESSINGMODE_DEFAULT, L"DEFAULT" },
    { AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS, L"COMMUNICATIONS" },
    { AUDIO_SIGNALPROCESSINGMODE_SPEECH, L"SPEECH" },
    { AUDIO_SIGNALPROCESSINGMODE_MEDIA, L"MEDIA" },
    { AUDIO_SIGNALPROCESSINGMODE_MOVIE, L"MOVIE" },
    { AUDIO_SIGNALPROCESSINGMODE_NOTIFICATION, L"NOTIFICATION" },
    { AUDIO_SIGNALPROCESSINGMODE_FAR_FIELD_SPEECH, L"FAR_FIELD_SPEECH" },
};


LPWSTR CPinTestResource::ModeName(REFGUID guidMode)
{
    for (auto m : knownModes)
    {
        if (m.mode == guidMode) { return m.name; }
    }

    return L"UNKNOWN";
}

LPWSTR CPinTestResource::PinName(EndpointConnectorType eConnectorType, bool IsMVA)
{
    switch (eConnectorType)
    {
    case eHostProcessConnector:
        return L"HOST";
    case eOffloadConnector:
        return L"OFFLOAD";
    case eLoopbackConnector:
        return L"LOOPBACK";
    case eKeywordDetectorConnector:
        return IsMVA?L"MVAKEYWORD":L"SVAKEYWORD";
    default:
        return L"UNKNOWN";
    }
}
