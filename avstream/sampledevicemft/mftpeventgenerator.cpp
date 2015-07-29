//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#include "stdafx.h"
#include "common.h"
#include "mftpeventgenerator.h"


#define TP_SCOPE_TRACE TP_NORMAL
#define DH_THIS_FILE DH_DEVPROXY

#ifdef MF_WPP
#include "mftpeventgenerator.tmh"
#endif

CMediaEventGenerator::CMediaEventGenerator () :
    m_nRefCount(0),
    m_pQueue(NULL),
    m_bShutdown(FALSE)
{
    //Call this explicit...
    InitMediaEventGenerator();
}

STDMETHODIMP CMediaEventGenerator::InitMediaEventGenerator(
    void
    )
{
    
    return MFCreateEventQueue(&m_pQueue);

}

STDMETHODIMP_(ULONG) CMediaEventGenerator::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG) CMediaEventGenerator::Release(
    void
    )
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);

    if (uCount == 0)
    {
        delete this;
    }
    return uCount;
}

STDMETHODIMP CMediaEventGenerator::QueryInterface(
    _In_ REFIID iid,
    _COM_Outptr_ void** ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if (iid == __uuidof(IUnknown) || iid == __uuidof(IMFMediaEventGenerator))
    {
        *ppv = static_cast<IMFMediaEventGenerator*>(this);
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
        DMFTCHECKHR_GOTO(hr, done);
    }

done:

    return hr;
}

//
// IMediaEventGenerator methods
//
STDMETHODIMP CMediaEventGenerator::BeginGetEvent(
    _In_ IMFAsyncCallback* pCallback,
    _In_ IUnknown* pState
    )
{
    HRESULT hr = S_OK;
    //MFWMITRACE(DH_THIS_FILE, TP_NORMAL, __FUNCTION__ " : MultiPinMFT BeginGetEvent called");
    m_critSec.Lock();

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = m_pQueue->BeginGetEvent(pCallback, pState);
    }

    m_critSec.Unlock();

    return hr;    
}

STDMETHODIMP CMediaEventGenerator::EndGetEvent(
    _In_ IMFAsyncResult* pResult,
    _Outptr_result_maybenull_ IMFMediaEvent** ppEvent
    )
{
    HRESULT hr = S_OK;
    //MFWMITRACE(DH_THIS_FILE, TP_NORMAL, __FUNCTION__ " : MultiPinMFT EndGetEvent called");
    m_critSec.Lock();

    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        hr = m_pQueue->EndGetEvent(pResult, ppEvent);
    }
    
    m_critSec.Unlock();
    
    return hr;    
}

STDMETHODIMP CMediaEventGenerator::GetEvent(
    _In_ DWORD dwFlags,
    _Outptr_result_maybenull_ IMFMediaEvent** ppEvent
    )
{
    //
    // Because GetEvent can block indefinitely, it requires
    // a slightly different locking strategy.
    //
    HRESULT hr = S_OK;
    //MFWMITRACE(DH_THIS_FILE, TP_NORMAL, __FUNCTION__ " : MultiPinMFT GetEvent called");
    IMFMediaEventQueue *pQueue = NULL;

    m_critSec.Lock();

    hr = CheckShutdown();
    //
    // Store the pointer in a local variable, so that another thread
    // does not release it after we leave the critical section.
    //
    if (SUCCEEDED(hr))
    {
        pQueue = m_pQueue;
    }
    
    m_critSec.Unlock();
    
    if (SUCCEEDED(hr))
    {
        hr = pQueue->GetEvent(dwFlags, ppEvent);
    }
        
    return hr;
}

STDMETHODIMP CMediaEventGenerator::QueueEvent(
    _In_ MediaEventType met,
    _In_ REFGUID extendedType,
    _In_ HRESULT hrStatus,
    _In_opt_ const PROPVARIANT* pvValue
    )
{
    HRESULT hr = S_OK;
    //MFWMITRACE(DH_THIS_FILE, TP_NORMAL, __FUNCTION__ " : MultiPinMFT QueueEvent called");
    m_critSec.Lock();
    
    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {

        hr = m_pQueue->QueueEventParamVar(
                            met, 
                            extendedType, 
                            hrStatus, 
                            pvValue
                            );
    }

    m_critSec.Unlock();
    
    return hr;
}

STDMETHODIMP CMediaEventGenerator::ShutdownEventGenerator(
    void
    )
{
    HRESULT hr = S_OK;

   // MFWMITRACE( DH_THIS_FILE, TP_LOWEST, __FUNCTION__ " : entering ...");

    m_critSec.Lock();
    
    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        if (m_pQueue)
        {
            hr = m_pQueue->Shutdown();
            //MFWMITRACE( DH_THIS_FILE, TP_NORMAL, __FUNCTION__ "Event Generator Queue shutdown hr = %x", hr);
        }
        SAFE_RELEASE(m_pQueue);
        m_bShutdown = TRUE;
    }
    m_critSec.Unlock();

   // MFWMITRACE( DH_THIS_FILE, TP_LOWEST, __FUNCTION__ " : exiting...");

    return hr;
}

STDMETHODIMP CMediaEventGenerator::QueueEvent(
    _In_ IMFMediaEvent* pEvent
    )
{
    HRESULT hr = S_OK;
    //MFWMITRACE(DH_THIS_FILE, TP_LOWEST, __FUNCTION__ " : QueueEvent 1 ...");
    m_critSec.Lock();
    
    hr = CheckShutdown();

    if (SUCCEEDED(hr))
    {
        if (m_pQueue)
        {
            hr = m_pQueue->QueueEvent(pEvent);
        }
    }

    m_critSec.Unlock();
    return hr;
}

CMediaEventGenerator::~CMediaEventGenerator (
    void
    )
{
    ShutdownEventGenerator();
    //MFASSERT(m_bShutdown);
    //MFASSERT(m_nRefCount == 0);
}
