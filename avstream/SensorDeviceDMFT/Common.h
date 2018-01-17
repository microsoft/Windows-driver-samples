//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#pragma once
#include "stdafx.h"
//////////////////////////////////////////////////////////////////////////
//  CCritSec
//  Description: Wraps a critical section.
//////////////////////////////////////////////////////////////////////////

#define SENSORDEVICEMFT_LEVEL_CRITICAL 0x1
#define SENSORDEVICEMFT_LEVEL_ERROR    0x2
#define SENSORDEVICEMFT_LEVEL_WARNING  0x3
#define SENSORDEVICEMFT_LEVEL_INFO     0x4
#define SENSORDEVICEMFT_LEVEL_VERBOSE  0x5

class CCritSec
{
private:
    CRITICAL_SECTION m_criticalSection;
public:
    CCritSec();
    ~CCritSec();
    _Requires_lock_not_held_(m_criticalSection) _Acquires_lock_(m_criticalSection)
        void Lock();
    _Requires_lock_held_(m_criticalSection) _Releases_lock_(m_criticalSection)
        void Unlock();
};

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)  { if(p) { (p)->Release(); (p)=NULL;} }
#endif // SAFE_RELEASE

#ifndef SAFE_CLOSE
#define SAFE_CLOSE(p)    { if(p) { CloseHandle(p); (p)=NULL;} }
#endif // SAFE_CLOSE

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)   { if(p) { delete(p); (p) = NULL;} }
#define SAFE_ARRAYDELETE(p)      { delete [] p; p = NULL;}
#endif //SAFE_DELETE

//////////////////////////////////////////////////////////////////////////
//  CAutoLock
//  Description: Provides automatic locking and unlocking of a 
//               of a critical section.
//////////////////////////////////////////////////////////////////////////

class CAutoLock
{
protected:
    CCritSec *m_pCriticalSection;
public:
    _Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
        CAutoLock(CCritSec& crit);
    _Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
        CAutoLock(CCritSec* crit);
    _Releases_lock_(this->m_pCriticalSection->m_criticalSection)
        ~CAutoLock();
};
//////////////////////////////////////////////////////////////////////////
// Just a friendly function that catches exceptions and returns an HRESULT
//////////////////////////////////////////////////////////////////////////

template <typename Lambda>
HRESULT ExceptionBoundary(Lambda&& lambda)
{
    try
    {
        lambda();
        return S_OK;
    }
    catch (const _com_error& e)
    {
        return e.Error();
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    catch (const std::out_of_range&)
    {
        return MF_E_INVALIDINDEX;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

//////////////////////////////////////////////////////////////////////////
// Singleton for Loading the MFSENSORGROUP.DLL
//////////////////////////////////////////////////////////////////////////
class CLoader{
public:
    static CLoader* GetLoader() 
    {
        static CLoader instance;
        return &instance;
    }
    HMODULE& GetModule()
    {
        return CLoader::g_lModule;
    }
 private:
    CLoader() throw(...)
    {
        g_lModule = LoadLibraryEx(TEXT("mfsensorgroup.dll"),0,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!g_lModule)
            throw exception("Library Load Failure");
    }
    static HMODULE g_lModule;
    ~CLoader()
    {
        if (g_lModule != NULL)
        {
            FreeLibrary(g_lModule);
            g_lModule = NULL;
        }
    }
    CLoader(CLoader&);              // Not implemented
    CLoader& operator=(CLoader&);   // Not implemented
};


//////////////////////////////////////////////////////////////////////////
// Module lifetime manager for keeping the COM DLL alive in memory when
// There are objects open on the DLL
//////////////////////////////////////////////////////////////////////////

class CDMFTModuleLifeTimeManager {
public:
    CDMFTModuleLifeTimeManager():m_pLoader(CLoader::GetLoader())
    {
        InterlockedIncrement(&s_lObjectCount);
    }
    ~CDMFTModuleLifeTimeManager()
    {
        InterlockedDecrement(&s_lObjectCount);
    }
    static long GetDMFTObjCount()
    {
        return s_lObjectCount;
    }
private:
    static volatile long s_lObjectCount;
    CLoader              *m_pLoader; // Stick in the Module life time manager here so that we will load the Sensor Group Library dynammically
};

//////////////////////////////////////////////////////////////////////////
// The below guid is used to register the GUID as the Sensor Transform. This should be adeed to the 
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceClasses\ and under the 
// GLOBAL#\Device Parameters key, add a "FSSensorTransformFactoryCLSID" value, and set its value to
// {BBC4DA67-7C37-4F6E-9E23-40CDDA51A621} for the Pipeline to pick up the Transform.
// One more key namely the FSSensorGroupID is added as a guid to all the sources that are a part of the
// Sensor Transform
//////////////////////////////////////////////////////////////////////////
DEFINE_GUID(CLSID_SensorTransformDeviceMFT,
    0xbbc4da67, 0x7c37, 0x4f6e, 0x9e, 0x23, 0x40, 0xcd, 0xda, 0x51, 0xa6, 0x21);

//////////////////////////////////////////////////////////////////////////
// Definition for sensorstream creation
//////////////////////////////////////////////////////////////////////////

typedef HRESULT(*PFNCREATESENSORSTREAM) (_In_ DWORD, _In_opt_ IMFAttributes*, _In_ IMFCollection*, _COM_Outptr_ IMFSensorStream**);

//////////////////////////////////////////////////////////////////////////
// Error check Maros.
//////////////////////////////////////////////////////////////////////////
#define SDMFTCHECKHR_GOTO(a,label) {if(FAILED(hr=a)) {\
   TraceLoggingWrite( \
   g_hSensorDeviceMFTProvider, \
   __FUNCTION__":ERROR",\
   TraceLoggingHResult(a) \
   );\
   goto label;\
}}
#define SDMFTCHECKNULL_GOTO(a,label,error) {if(!a) {\
   hr = error;\
   TraceLoggingWrite( \
   g_hSensorDeviceMFTProvider, \
   __FUNCTION__":NULL",\
   TraceLoggingHResult(error) \
   );\
   goto label;\
}}
TRACELOGGING_DECLARE_PROVIDER(g_hSensorDeviceMFTProvider);
//////////////////////////////////////////////////////////////////////////
// Trace Helper definitions
//////////////////////////////////////////////////////////////////////////
VOID PRINTTRACE(_In_ LPCWCHAR lpFunc, _In_ ULONG line, _In_ ULONG level, _In_ LPCWCHAR lpFormatStr, ...);

#define SDMFTTRACE(level,str,...) { \
    PRINTTRACE(__FUNCTIONW__,__LINE__,level,L#str,__VA_ARGS__);\
}

//
// Keeps track of Sensor MFT's state change operations
//
class CTransformStateOperation {
public:
    CTransformStateOperation() : m_dwInPinId(DWORD_MAX),
        m_dwOutPinId(DWORD_MAX),
        m_bStateOperationActive(FALSE),
        m_hEvent(NULL),
        m_hShutdown(NULL)
    {}
    ~CTransformStateOperation() {
        SAFE_CLOSE(m_hEvent);
        SAFE_CLOSE(m_hShutdown);
    }
    HRESULT Start(_In_ DWORD dwPinId, _In_ DWORD dwOutPinId,DeviceStreamState state, _In_ IMFMediaType* pMediaType) {
        // Put validation for Input Pins and Output Pins here
        HRESULT hr = S_OK;
        CAutoLock Lock(m_Lock);
        if (!m_bStateOperationActive)
        {
            m_dwInPinId = dwPinId;
            m_dwOutPinId = dwOutPinId;
            m_spPreferredMediaType = pMediaType;
            m_DestState = state;
            m_bStateOperationActive = TRUE;
            if (!m_hEvent)
            {
                m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                m_hShutdown = CreateEvent(NULL, TRUE, FALSE, NULL); //Set to FALSE unless shutdown
                if (!m_hEvent || !m_hShutdown)
                {
                    m_bStateOperationActive = FALSE;
                    m_spPreferredMediaType = nullptr;
                    m_dwOutPinId = (m_dwInPinId = DWORD_MAX);
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    ResetEvent(m_hEvent);
                }
            }
            
        }
        else
        {
            hr = MF_E_INVALIDREQUEST;
        }
        if (FAILED(hr))
        {
            SAFE_CLOSE(m_hEvent);
            SAFE_CLOSE(m_hShutdown);
        }
        return hr;
    }
    HRESULT Finish( _In_ DWORD dwInputPinId )
    {
        HRESULT hr = S_OK;
        CAutoLock Lock(m_Lock);
        if (dwInputPinId != m_dwInPinId)
        {
            // Make sure we have got the right call from the Transform Manager. Always called from Input Pin
            hr = MF_E_INVALIDSTREAMNUMBER;
        }
        else
        {
            SetEvent(m_hEvent);
        }
        return hr;
    }
    HRESULT GetMediaTypeForInput(_In_ DWORD dwPinId, _Inout_ IMFMediaType** ppMediaType, _In_ DeviceStreamState &state) {
        // Only if a state operation is in progress answer a valid response
        CAutoLock Lock(m_Lock);
        if (m_bStateOperationActive && (dwPinId == m_dwInPinId))
        {
            if (m_spPreferredMediaType.Get())
            {
                m_spPreferredMediaType.CopyTo(ppMediaType);
            }
            state = m_DestState;
            return S_OK;
        }
        return MF_E_INVALIDREQUEST;
    }
    BOOL IsActive()
    {
        return m_bStateOperationActive;
    }
    HRESULT ShutDown()
    {
        CAutoLock Lock(m_Lock);
        SetEvent(m_hShutdown);
        return S_OK;
    }
    _Requires_lock_not_held_(m_Lock)
    HRESULT Wait()
    {
        HRESULT hr = S_OK;
        HANDLE hEvents[] = { m_hEvent, m_hShutdown };
        if ( m_bStateOperationActive /*State operations active*/)
        {
            DWORD dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
            switch (dwRet)
            {
            case WAIT_OBJECT_0:     /*Success*/ 
                break;
            case (WAIT_OBJECT_0+1): /*Shutdown*/ 
                hr = MF_E_SHUTDOWN;
                break;
            case WAIT_TIMEOUT:      /*Timeout*/
                hr = MF_E_HW_MFT_FAILED_START_STREAMING; /*Ol Faithful*/
                m_bStateOperationActive = FALSE;
                break;
            }
            if (SUCCEEDED(hr))
            {
                // Clear the state and return
                m_dwInPinId = (m_dwOutPinId = DWORD_MAX);
                m_spPreferredMediaType = nullptr;
                m_bStateOperationActive = FALSE;
            }
        }
        else
        {
            hr = MF_E_INVALIDREQUEST; /*Invalid State for a wait*/
        }
       return hr;
    }
private:
    DWORD  m_dwInPinId; //The Sample has only one input pin connected. If multiple input pins are connected, this should be an array of input pins connected to the output pin
    DWORD  m_dwOutPinId;
    ComPtr<IMFMediaType> m_spPreferredMediaType;
    DeviceStreamState    m_DestState;
    BOOL   m_bStateOperationActive;
   HANDLE m_hEvent;
    HANDLE m_hShutdown;
    CCritSec m_Lock;
};
class CMediaEventGenerator :public IMFMediaEventGenerator{
public:
     //////////////////////////////////////////////////////////////////////////
    /// IMFMediaEventGenerator 
    //////////////////////////////////////////////////////////////////////////
    IFACEMETHOD(BeginGetEvent)(_In_ IMFAsyncCallback* pCallback, _In_ IUnknown* pState);
    IFACEMETHOD(EndGetEvent)(_In_ IMFAsyncResult* pResult, _COM_Outptr_ IMFMediaEvent** ppEvent);
    IFACEMETHOD(GetEvent)(_In_ DWORD dwFlags, _COM_Outptr_ IMFMediaEvent** ppEvent);
    IFACEMETHOD(QueueEvent)(_In_ MediaEventType met, _In_ REFGUID extendedType, _In_ HRESULT hrStatus, _In_opt_ const PROPVARIANT* pvValue);
    //////////////////////////////////////////////////////////////////////////
    // IUnknown implementation
    //////////////////////////////////////////////////////////////////////////
public:
    STDMETHODIMP_(ULONG) CMediaEventGenerator::AddRef(VOID)
    {
        return InterlockedIncrement(&m_nRefCount);
    }
    STDMETHODIMP_(ULONG) CMediaEventGenerator::Release(VOID)
    {
        ULONG uCount = InterlockedDecrement(&m_nRefCount);
        if (uCount == 0)
        {
            delete this;
        }
        return uCount;
    }
    STDMETHODIMP CMediaEventGenerator::QueryInterface(
        __in REFIID iid,
        __deref_out void** ppv
    )
    {
        HRESULT hr = S_OK;
        if (ppv != NULL) {
            *ppv = NULL;
            if (iid == __uuidof(IUnknown) || iid == __uuidof(IMFMediaEventGenerator))
            {
                *ppv = static_cast<IMFMediaEventGenerator*>(this);
                AddRef();
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        return hr;
    }
    CMediaEventGenerator::CMediaEventGenerator() : m_nRefCount(0), m_bShutdown(FALSE) 
    {
        HRESULT hr = InitMediaEventGenerator();
        //
        // Make sure we throw an exception when we fail. We are always catching the exception from the 
        // inheritor class in thie SAMPLE so we should be fine if this fails
        //
        if (FAILED(hr))
        {
            throw bad_alloc();
        }
    }
protected:
    virtual ~CMediaEventGenerator(VOID)
    {
        ShutdownEventGenerator();
    }
    //
    // Utility functions
    //
    STDMETHOD(QueueEvent)(
        _In_ IMFMediaEvent* pEvent
        );
    STDMETHODIMP CMediaEventGenerator::InitMediaEventGenerator(VOID)
    {
        return MFCreateEventQueue(m_spEventQueue.GetAddressOf());
    }
    __inline HRESULT(CheckShutdown)(VOID) const
    {
        return (m_bShutdown ? MF_E_SHUTDOWN : S_OK);
    }
    STDMETHOD(ShutdownEventGenerator)(VOID);
    
private:
    LONG                m_nRefCount;
    CCritSec            m_critSec;
    BOOL                m_bShutdown;
    ComPtr<IMFMediaEventQueue>  m_spEventQueue;
};



