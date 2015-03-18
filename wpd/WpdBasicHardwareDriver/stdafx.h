#pragma once

#include "resource.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define STRSAFE_NO_DEPRECATE

#include <stdio.h>
#include <tchar.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlstr.h>
#include <strsafe.h>

// This driver is entirely user-mode
_Analysis_mode_(_Analysis_code_type_user_code_);

//
// Driver specific tracing #defines

//
// TODO: Change these values to be appropriate for your driver.
//
#define MYDRIVER_TRACING_ID      L"Microsoft\\WPD\\BasicHardwareDriver"

//
// TODO: Choose a different trace control GUID
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(BasicHardwareDriverCtlGuid,(80068de5,197d,4db6,b77c,29ee3cca4537), \
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


#include <PortableDevice.h>
#include <PortableDeviceTypes.h>
#include <PortableDeviceClassExtension.h>

#include <initguid.h>   // Required to access the DEFINE_GUID macro
#include <propkeydef.h> // Required to access the DEFINE_PROPERTYKEY macro

// {CDD18979-A7B0-4D5E-9EB2-0A826805CBBD}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_WUDF_DEVICE_OBJECT, 0xCDD18979, 0xA7B0, 0x4D5E, 0x9E, 0xB2, 0x0A, 0x82, 0x68, 0x05, 0xCB, 0xBD, 2);
// {9BD949E5-59CF-41AE-90A9-BE1D044F578F}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_WPD_SERIALIZER_OBJECT, 0x9BD949E5, 0x59CF, 0x41AE, 0x90, 0xA9, 0xBE, 0x1D, 0x04, 0x4F, 0x57, 0x8F, 2);
// {4DF6C8C7-2CE5-457C-9F53-EFCECAA95C04}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, 0x4DF6C8C7, 0x2CE5, 0x457C, 0x9F, 0x53, 0xEF, 0xCE, 0xCA, 0xA9, 0x5C, 0x04, 2);

/**************************************************************************** 
* This section defines the Functional Category associated with the sensor object
****************************************************************************/ 

// 
// FUNCTIONAL_CATEGORY_SENSOR_SAMPLE
//   Indicates this object encapsulates sensor functionality on the device
DEFINE_GUID (FUNCTIONAL_CATEGORY_SENSOR_SAMPLE, 0x09b93609, 0xd5c1,0x497b,0xb2, 0x52, 0xb4, 0x92, 0xcb, 0x5e, 0xfe, 0x52);


/**************************************************************************** 
* This section defines all Commands, Parameters and Options associated with: 
* SENSOR_PROPERTIES_V1 
* 
* This category is for properties common to all sensor objects. 
****************************************************************************/ 
DEFINE_GUID (SENSOR_PROPERTIES_V1, 0xa7ef4367, 0x6550, 0x4055, 0xb6, 0x6f, 0xbe, 0x6f, 0xda, 0xcf, 0x4e, 0x9f);

// 
// SENSOR_READING 
//   [ VT_UI4 ] Indicates the sensor reading in degrees Kelvin.
DEFINE_PROPERTYKEY(SENSOR_READING, 0xa7ef4367, 0x6550, 0x4055, 0xb6, 0x6f, 0xbe, 0x6f, 0xda, 0xcf, 0x4e, 0x9f, 2);
//
// SENSOR_UPDATE_INTERVAL 
//   [ VT_UI4 ] Indicates the sensor update interval in milliseconds. 
DEFINE_PROPERTYKEY(SENSOR_UPDATE_INTERVAL, 0xa7ef4367, 0x6550, 0x4055, 0xb6, 0x6f, 0xbe, 0x6f, 0xda, 0xcf, 0x4e, 0x9f, 3);

/**************************************************************************** 
* This section defines all Events associated with the sensor object 
****************************************************************************/ 
// 
// EVENT_SENSOR_READING_UPDATED
//   This event is sent after a new sensor reading is available on the device. 
DEFINE_GUID (EVENT_SENSOR_READING_UPDATED, 0xada23b0b, 0xce13, 0x4e11, 0x9d, 0x2f, 0x15, 0xfe, 0x10, 0xd6, 0x63, 0x37);


//
// Macro Definitions
//

#ifndef SAFE_RELEASE
    #define SAFE_RELEASE(p) if( NULL != p ) { ( p )->Release(); p = NULL; }
#endif

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


// Forward declaration of WpdBaseDriver as it is accessed by other classes
class WpdBaseDriver;

#include "WpdBasicHardwareDriver.h"
#include "RS232Connection.h"
#include "RS232Target.h"
#include "WpdObjectEnum.h"
#include "WpdObjectProperties.h"
#include "WpdCapabilities.h"
#include "WpdBaseDriver.h"
#include "Device.h"
#include "Driver.h"
#include "Queue.h"

extern HINSTANCE g_hInstance;

