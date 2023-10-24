//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#pragma once

#ifndef MF_WPP
#define DMFTRACE(...) 
#endif

//
// The below guid is used to register the GUID as the Device Transform. This should be adeed to the 
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceClasses\ and under the 
// GLOBAL#\Device Parameters key, add a CameraDeviceMFTCLSID value, and set its value to
// {836E84ED-45E9-4160-A79D-771F1C718CD2} for the Pipeline to pick up the Transform.
//

DEFINE_GUID(CLSID_AvsCameraDMFT, 0x836e84ed, 0x45e9, 0x4160, 0xa7, 0x9d, 0x77, 0x1f, 0x1c, 0x71, 0x8c, 0xd2);

#ifdef DBG
#define mf_assert(a) if(!a) DebugBreak()
#else
#define mf_assert(a)
#endif

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGUID_DMFTTrace, (CBCCA12E, 9472, 409D, A1B1, 753C98BF03C0), \
    WPP_DEFINE_BIT(DMFT_INIT) \
    WPP_DEFINE_BIT(DMFT_CONTROL) \
    WPP_DEFINE_BIT(DMFT_GENERAL) \
    )

#define WPP_LEVEL_FLAG_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)  
#define WPP_LEVEL_FLAG_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)  
#define WPP_FLAG_LEVEL_LOGGER(flags,lvl) WPP_LEVEL_LOGGER(flags)  
#define WPP_FLAG_LEVEL_ENABLED(flags, lvl) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)  

#define WPP_CHECK_LEVEL_ENABLED(flags, level) 1

//begin_wpp config
//
// USEPREFIX (DMFTCHECKNULL_GOTO,"%!STDPREFIX!");
// FUNC DMFTCHECKNULL_GOTO(CHECKNULLGOTO_EXP,LABEL,HR,...);
// USESUFFIX (DMFTCHECKNULL_GOTO," failed %!HRESULT!\n", hr);
//
//end_wpp

//begin_wpp config
//
// USEPREFIX (DMFTCHECKHR_GOTO,"%!STDPREFIX!");
// FUNC DMFTCHECKHR_GOTO(CHECKHRGOTO_EXP,LABEL,...);
// USESUFFIX (DMFTCHECKHR_GOTO," failed %!HRESULT!\n", hr);
//

//end_wpp

#define WPP_CHECKNULLGOTO_EXP_LABEL_HR_PRE(pointer,label,HR) if( pointer == NULL ) { hr = HR;
#define WPP_CHECKNULLGOTO_EXP_LABEL_HR_POST(pointer,label,HR) ; goto label; }

#define WPP_CHECKNULLGOTO_EXP_LABEL_HR_LOGGER(pointer,label,HR) WPP_LEVEL_LOGGER( DMFT_INIT )
#define WPP_CHECKNULLGOTO_EXP_LABEL_HR_ENABLED(pointer,label,HR) WPP_CHECK_LEVEL_ENABLED( DMFT_INIT, TP_ERROR )

#define WPP_CHECKHRGOTO_EXP_LABEL_PRE(HR,label) if( FAILED( hr = HR ) ) {
#define WPP_CHECKHRGOTO_EXP_LABEL_POST(HR,label) ; goto label; }

#define WPP_CHECKHRGOTO_EXP_LABEL_LOGGER(HR, label) WPP_LEVEL_LOGGER( DMFT_INIT )
#define WPP_CHECKHRGOTO_EXP_LABEL_ENABLED(HR, label) WPP_CHECK_LEVEL_ENABLED( DMFT_INIT, TP_ERROR )


// Give it checkhr and checknull definitions if some future generations of visual studio remove the wpp processor support

//#if !defined DMFTCHECKHR_GOTO
//#define DMFTCHECKHR_GOTO(a,b) {hr=(a); if(FAILED(hr)){goto b;}} 
//#endif
//
//#if !defined DMFTCHECKNULL_GOTO
//#define DMFTCHECKNULL_GOTO(a,b,c) {if(!a) {hr = c; goto b;}} 
//#endif

#define SAFE_ADDREF(p)              if( NULL != p ) { ( p )->AddRef(); }
#define SAFE_DELETE(p)              delete p; p = NULL;
#define SAFE_SHUTDELETE(p)          if( NULL != p ) { ( p )->Shutdown(); delete p; p = NULL; }
#define SAFE_RELEASE(p)             if( NULL != p ) { ( p )->Release(); p = NULL; }
#define SAFE_SHUTRELEASE(p)         if( NULL != p ) { ( p )->Shutdown(); ( p )->Release(); p = NULL; }
#define SAFE_CLOSERELEASE(p)        if( NULL != p ) { ( p )->Close( TRUE ); ( p )->Release(); p = NULL; }
#define SAFE_COTASKMEMFREE(p)       CoTaskMemFree( p ); p = NULL;
#define SAFE_SYSFREESTRING(p)       SysFreeString( p ); p = NULL;
#define SAFE_ARRAYDELETE(p)         delete [] p; p = NULL;
#define SAFE_BYTEARRAYDELETE(p)     delete [] (BYTE*) p; p = NULL;
#define SAFE_CLOSEHANDLE(h)         { if(INVALID_HANDLE_VALUE != (h)) { ::CloseHandle(h); (h) = INVALID_HANDLE_VALUE; } }


#define SAFERELEASE(x) \
if (x) {\
    x->Release(); \
    x = NULL; \
}

#if !defined(_IKsControl_)
#define _IKsControl_
interface DECLSPEC_UUID("28F54685-06FD-11D2-B27A-00A0C9223196") IKsControl;
#undef INTERFACE
#define INTERFACE IKsControl
DECLARE_INTERFACE_(IKsControl, IUnknown)
{
    STDMETHOD(KsProperty)(
        THIS_
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
    STDMETHOD(KsMethod)(
        THIS_
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
    STDMETHOD(KsEvent)(
        THIS_
        IN PKSEVENT Event OPTIONAL,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
};
#endif //!defined(_IKsControl_)

//Forward defintion
class CBasePin;
//////////////////////////////////////////////////////////////////////////
//  CCritSec
//  Description: Wraps a critical section.
//////////////////////////////////////////////////////////////////////////

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

class MediaBufferLock
{
public:
    MediaBufferLock(_In_ IMFMediaBuffer* pBuffer) :
        m_bLocked(false)
    {
        m_spBuffer = pBuffer;
    }

    HRESULT LockBuffer(
        _Outptr_result_bytebuffer_to_(*pcbMaxLength, *pcbCurrentLength)  BYTE** ppbBuffer,
        _Out_opt_  DWORD* pcbMaxLength,
        _Out_opt_  DWORD* pcbCurrentLength)
    {
        if (!m_spBuffer)
        {
            return E_INVALIDARG;
        }

        HRESULT hr = m_spBuffer->Lock(ppbBuffer, pcbMaxLength, pcbCurrentLength);
        if (FAILED(hr))
        {
            return hr;
        }
        m_bLocked = true;
        return S_OK;
    }

    ~MediaBufferLock()
    {
        if (m_spBuffer && m_bLocked)
        {
            //Unlock fails only if we did not lock it first
            (void)m_spBuffer->Unlock();
        }
    }

private:
    ComPtr<IMFMediaBuffer> m_spBuffer;
    bool m_bLocked;
};

typedef  std::vector<ComPtr<IMFMediaType>> IMFMediaTypeArray;
typedef  std::vector<ComPtr<CBasePin>>     CBasePinArray;
typedef  std::vector<ComPtr<IMFSample>>    IMFSampleList;
typedef  std::pair< std::multimap<int, int>::iterator, std::multimap<int, int>::iterator > MMFTMMAPITERATOR;

STDMETHODIMP_(BOOL) IsPinStateInActive(
    _In_ DeviceStreamState state
    );


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
        return E_UNEXPECTED;
    }
}

//
// Object LifeTime manager. The Class has a global variable which
// maintains a reference count of the number of objects in the
// system managed by the DLL. 
//
class CDMFTModuleLifeTimeManager{
public:
    CDMFTModuleLifeTimeManager()
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
};

class CPinQueue : public IUnknown
{
public:
    CPinQueue(_In_ DWORD _inPinId, _In_ IMFDeviceTransform* pTransform = nullptr);
    ~CPinQueue();

    STDMETHODIMP Insert(_In_ IMFSample* pSample);
    STDMETHODIMP Remove(_Outptr_result_maybenull_ IMFSample** pSample);
    STDMETHODIMP_(VOID) Clear();

    //
    // Inline functions
    //
    __inline BOOL Empty()
    {
        return (!m_sampleList.size());
    }
    __inline DWORD pinStreamId()
    {
        return m_dwInPinId;
    }
    __inline GUID pinCategory()
    {
        if (IsEqualCLSID(m_streamCategory, GUID_NULL))
        {
            ComPtr<IMFAttributes> spAttributes;
            if (SUCCEEDED(m_pTransform->GetOutputStreamAttributes(pinStreamId(), spAttributes.ReleaseAndGetAddressOf())))
            {
                (VOID) spAttributes->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &m_streamCategory);
            }
        }
        return m_streamCategory;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        HRESULT hr = S_OK;
        if (ppv != nullptr)
        {
            *ppv = nullptr;
            if (riid == __uuidof(IUnknown))
            {
                AddRef();
                *ppv = static_cast<IUnknown*>(this);
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }
        else
        {
            hr = E_POINTER;
        }
        return hr;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }
    STDMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }

private:
    DWORD m_dwInPinId;                /* This is the input pin       */
    IMFSampleList m_sampleList;       /* List storing the samples    */
    IMFDeviceTransform* m_pTransform; /* Weak reference to the the device MFT */
    GUID m_streamCategory;
    ULONG m_cRef;
};


HRESULT ProcessMetadata(_In_ IMFSample* pSample);
// Metadata defintions

