#pragma once

#include "resource.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define STRSAFE_NO_DEPRECATE

#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlstr.h>

#ifndef SAFE_RELEASE
    #define SAFE_RELEASE(p) if( NULL != p ) { ( p )->Release(); p = NULL; }
#endif

#include "WpdHelloWorldDriver.h"
#include "PortableDeviceTypes.h"
#include "PortableDeviceClassExtension.h"
#include "PortableDevice.h"

#include <initguid.h>
#include <propkeydef.h>

// {CDD18979-A7B0-4D5E-9EB2-0A826805CBBD}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_WUDF_DEVICE_OBJECT, 0xCDD18979, 0xA7B0, 0x4D5E, 0x9E, 0xB2, 0x0A, 0x82, 0x68, 0x05, 0xCB, 0xBD, 2);
// {9BD949E5-59CF-41AE-90A9-BE1D044F578F}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_WPD_SERIALIZER_OBJECT, 0x9BD949E5, 0x59CF, 0x41AE, 0x90, 0xA9, 0xBE, 0x1D, 0x04, 0x4F, 0x57, 0x8F, 2);
// {4DF6C8C7-2CE5-457C-9F53-EFCECAA95C04}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, 0x4DF6C8C7, 0x2CE5, 0x457C, 0x9F, 0x53, 0xEF, 0xCE, 0xCA, 0xA9, 0x5C, 0x04, 2);

class ContextMap : public IUnknown
{
public:
    ContextMap() :
        m_cRef(1)
    {

    }

    ~ContextMap()
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);

        IUnknown*   pUnk            = NULL;
        POSITION    elementPosition = NULL;

        elementPosition = m_Map.GetStartPosition();
        while(elementPosition != NULL)
        {
            pUnk = m_Map.GetNextValue(elementPosition);
            if(pUnk != NULL)
            {
                pUnk->Release();
            }
        }
    }

public: // IUnknown
    ULONG __stdcall AddRef()
    {
        InterlockedIncrement((long*) &m_cRef);
        return m_cRef;
    }

    _At_(this, __drv_freesMem(Mem)) 
    ULONG __stdcall Release()
    {
        ULONG ulRefCount = m_cRef - 1;

        if (InterlockedDecrement((long*) &m_cRef) == 0)
        {
            delete this;
            return 0;
        }
        return ulRefCount;
    }

    HRESULT __stdcall QueryInterface(
        REFIID riid,
        void** ppv)
    {
        HRESULT hr = S_OK;

        if(riid == IID_IUnknown)
        {
            *ppv = static_cast<IUnknown*>(this);
            AddRef();
        }
        else
        {
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
        return hr;
    }


public: // Context accessor methods

    // If successful, this method AddRef's the context and returns
    // a context key
    HRESULT Add(
        _In_  IUnknown*     pContext,
        _Out_ CAtlStringW&  key)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        HRESULT  hr          = S_OK;
        GUID     guidContext = GUID_NULL;
        CComBSTR bstrContext;

        key = L"";

        // Create a unique context key
        hr = CoCreateGuid(&guidContext);
        if (hr == S_OK)
        {
            bstrContext = guidContext;
            if(bstrContext.Length() > 0)
            {
                key = bstrContext;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (hr == S_OK)
        {
            // Insert this into the map
            POSITION  elementPosition = m_Map.SetAt(key, pContext);
            if(elementPosition != NULL)
            {
                // AddRef since we are holding onto it
                pContext->AddRef();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        return hr;
    }

    void Remove(
        _In_ const CAtlStringW&  key)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        // Get the element
        IUnknown* pContext = NULL;

        if (m_Map.Lookup(key, pContext) == true)
        {
            // Remove the entry for it
            m_Map.RemoveKey(key);

            // Release it
            pContext->Release();
        }
    }

    // Returns the context pointer.  If not found, return value is NULL.
    // If non-NULL, caller is responsible for Releasing when it is done,
    // since this method will AddRef the context.
    IUnknown* GetContext(
        _In_ const CAtlStringW&  key)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        // Get the element
        IUnknown* pContext = NULL;

        if (m_Map.Lookup(key, pContext) == true)
        {
            // AddRef
            pContext->AddRef();
        }
        return pContext;
    }

private:
    CComAutoCriticalSection         m_CriticalSection;
    CAtlMap<CAtlStringW, IUnknown*> m_Map;
    DWORD                           m_cRef;
};

HRESULT UpdateDeviceFriendlyName(
    _In_ IPortableDeviceClassExtension*  pPortableDeviceClassExtension,
    _In_ LPCWSTR                         wszDeviceFriendlyName);

#include "WpdObjectEnum.h"
#include "WpdObjectProperties.h"
#include "WpdObjectResources.h"
#include "WpdCapabilities.h"
#include "WpdBaseDriver.h"

extern HINSTANCE g_hInstance;

//
// Driver specific tracing #defines
//
// TODO: Change these values to be appropriate for your driver.
//
#define MYDRIVER_TRACING_ID      L"Microsoft\\WPD\\HelloWorldDriver"

//
// TODO: Choose a different trace control GUID
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(HelloWorldDriverCtlGuid,(607517b9,d810,4074,b4c2,cb7d950c2c32), \
        WPP_DEFINE_BIT(TRACE_FLAG_ALL)                                      \
        WPP_DEFINE_BIT(TRACE_FLAG_DEVICE)                                   \
        WPP_DEFINE_BIT(TRACE_FLAG_DRIVER)                                   \
        WPP_DEFINE_BIT(TRACE_FLAG_QUEUE)                                    \
        )     

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// TraceEvents function.
//
// begin_wpp config
// FUNC Trace{FLAG=TRACE_FLAG_ALL}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp

//
// This comment block is scanned by the trace preprocessor to define our
// CHECK_HR function.
//
//
// begin_wpp config
// USEPREFIX (CHECK_HR,"%!STDPREFIX!");
// FUNC CHECK_HR{FLAG=TRACE_FLAG_ALL}(hrCheck, MSG, ...);
// USESUFFIX (CHECK_HR, " hr= %!HRESULT!", hrCheck);
// end_wpp

//
// PRE macro: The name of the macro includes the condition arguments FLAGS and EXP
//            define in FUNC above
//
#define WPP_FLAG_hrCheck_PRE(FLAGS, hrCheck) {if(hrCheck != S_OK) {

//
// POST macro
// The name of the macro includes the condition arguments FLAGS and EXP
//            define in FUNC above
#define WPP_FLAG_hrCheck_POST(FLAGS, hrCheck) ; } }

// 
// The two macros below are for checking if the event should be logged and for 
// choosing the logger handle to use when calling the ETW trace API
//
#define WPP_FLAG_hrCheck_ENABLED(FLAGS, hrCheck) WPP_FLAG_ENABLED(FLAGS)
#define WPP_FLAG_hrCheck_LOGGER(FLAGS, hrCheck) WPP_FLAG_LOGGER(FLAGS)
