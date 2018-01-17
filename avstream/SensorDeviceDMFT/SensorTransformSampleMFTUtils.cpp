//*@@@+++@@@@******************************************************************
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//


#include "stdafx.h"
#include "common.h"


// Critical sections

CCritSec::CCritSec()
{
    InitializeCriticalSection(&m_criticalSection);
}

CCritSec::~CCritSec()
{
    DeleteCriticalSection(&m_criticalSection);
}

_Requires_lock_not_held_(m_criticalSection) _Acquires_lock_(m_criticalSection)
void CCritSec::Lock()
{
    EnterCriticalSection(&m_criticalSection);
}

_Requires_lock_held_(m_criticalSection) _Releases_lock_(m_criticalSection)
void CCritSec::Unlock()
{
    LeaveCriticalSection(&m_criticalSection);
}


_Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::CAutoLock(CCritSec& crit)
{
    m_pCriticalSection = &crit;
    m_pCriticalSection->Lock();
}
_Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::CAutoLock(CCritSec* crit)
{
    m_pCriticalSection = crit;
    m_pCriticalSection->Lock();
}
_Releases_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::~CAutoLock()
{
    m_pCriticalSection->Unlock();
}

//////////////////////////////////////////////////////////////////////////
// IMFMediaEventGenerator 
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP
CMediaEventGenerator::BeginGetEvent(
    _In_ IMFAsyncCallback* pCallback,
    _In_ IUnknown* pState
)
{
    HRESULT hr = S_OK;
    CAutoLock lock(&m_critSec);
    SDMFTCHECKHR_GOTO(CheckShutdown(), done);
    SDMFTCHECKHR_GOTO(m_spEventQueue->BeginGetEvent(pCallback, pState), done);

done:
    return hr;
}

IFACEMETHODIMP
CMediaEventGenerator::EndGetEvent(
    _In_ IMFAsyncResult* pResult,
    _COM_Outptr_ IMFMediaEvent** ppEvent
)
{
    HRESULT hr = S_OK;
    CAutoLock lock(&m_critSec);
    SDMFTCHECKHR_GOTO(CheckShutdown(), done);
    SDMFTCHECKHR_GOTO(m_spEventQueue->EndGetEvent(pResult, ppEvent), done);
done:
    return hr;
}

IFACEMETHODIMP
CMediaEventGenerator::GetEvent(
    _In_ DWORD dwFlags,
    _COM_Outptr_ IMFMediaEvent** ppEvent
)
{
    HRESULT hr = S_OK;
    CAutoLock lock(&m_critSec);
    SDMFTCHECKHR_GOTO(CheckShutdown(), done);
    SDMFTCHECKHR_GOTO(m_spEventQueue->GetEvent(dwFlags, ppEvent), done);
done:
    return hr;
}

IFACEMETHODIMP
CMediaEventGenerator::QueueEvent(
    _In_ MediaEventType met,
    _In_ REFGUID extendedType,
    _In_ HRESULT hrStatus,
    _In_opt_ const PROPVARIANT* pvValue
)
{
    HRESULT hr = S_OK;
    CAutoLock lock(&m_critSec);
    SDMFTCHECKHR_GOTO(CheckShutdown(), done);
    SDMFTCHECKHR_GOTO(m_spEventQueue->QueueEventParamVar(met, extendedType, hrStatus, pvValue), done);
done:
    return hr;
}

STDMETHODIMP CMediaEventGenerator::ShutdownEventGenerator(VOID)
{
    HRESULT hr = S_OK;
    CAutoLock lock(&m_critSec);
    SDMFTCHECKHR_GOTO(CheckShutdown(),done);
    if (m_spEventQueue)
    {
        SDMFTCHECKHR_GOTO(m_spEventQueue->Shutdown(),done);
    }
    m_bShutdown = TRUE;
done:
    return hr;
}
STDMETHODIMP CMediaEventGenerator::QueueEvent(
    __in IMFMediaEvent* pEvent
)
{
    HRESULT hr = S_OK;
    CAutoLock lock(&m_critSec);
    SDMFTCHECKHR_GOTO(CheckShutdown(), done);
    if (m_spEventQueue)
    {
        hr = m_spEventQueue->QueueEvent(pEvent);
    }
done:
    return hr;
}


VOID PRINTTRACE( _In_ LPCWCHAR lpFunc, _In_ ULONG line,  _In_ const ULONG level, _In_ LPCWCHAR lpFormatStr, ...)
{
    HRESULT hr      = S_OK;
    size_t cchSize  = 0;
    CONST size_t max_str_length = 256;// If we exceed this limit, we skip tracing!
    WCHAR wFormattedString[max_str_length]; // On the stack.. need to check what is safe
    va_list vaArgs;
    UNREFERENCED_PARAMETER(level);

    va_start(vaArgs, lpFormatStr);
    SDMFTCHECKHR_GOTO(StringCchLength(lpFormatStr, max_str_length, &cchSize),done);
    SDMFTCHECKHR_GOTO(StringCbVPrintfEx(wFormattedString,max_str_length,NULL,NULL, STRSAFE_NULL_ON_FAILURE,lpFormatStr,vaArgs),done);
    TraceLoggingWrite(g_hSensorDeviceMFTProvider,"Trace:",
       TraceLoggingLevel(SENSORDEVICEMFT_LEVEL_INFO),
        TraceLoggingWideString(lpFunc,":"),
        TraceLoggingInt32(line," "),
        TraceLoggingWideString(wFormattedString,":"));
done:
    va_end(vaArgs);
    if (FAILED(hr))
    {
        // Failed tracing Log the failure so the error is known
        TraceLoggingWrite(g_hSensorDeviceMFTProvider, __FUNCTION__"Failed Logging Trace at Line", 
            TraceLoggingLevel(SENSORDEVICEMFT_LEVEL_ERROR),
            TraceLoggingInt32(line),
            TraceLoggingHResult(hr));
    }
}

