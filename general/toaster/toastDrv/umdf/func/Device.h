/*++

  Copyright (c) Microsoft Corporation, All Rights Reserved

  Module Name:

    Device.h

  Abstract:

    This file contains the class definition for device callback object.

  Environment:

    Windows User-Mode Driver Framework (UMDF)

--*/

#pragma once
#include "resource.h"
#include "WUDFToaster.h"

//
// To inform the framework about the callbacks we are interested in, we
// simply derive from the desired set of interfaces.
//
class CDevice : public IPnpCallbackHardware
{
public:
    CDevice() : m_cRefs(0)
    {
    }

    
public:

    //
    // Static method that creates a device callback object.
    //
    static HRESULT CreateInstance(_Out_ IUnknown ** ppUnkwn)
    {
        *ppUnkwn = NULL;
        
#pragma warning( suppress : 6014 )// PFD ISSUE: counted memory locks
        CDevice *pMyDevice = new CDevice();
        
        if (NULL == pMyDevice)
        {
            return E_OUTOFMEMORY;
        }

        return (pMyDevice->QueryInterface( __uuidof(IUnknown), (void **) ppUnkwn ));
    }

    //
    // IUnknown
    //
    virtual HRESULT __stdcall QueryInterface(_In_ REFIID riid, _Out_ LPVOID* ppvObject);
    virtual ULONG   __stdcall AddRef();
    _At_(this, __drv_freesMem(object))
    virtual ULONG   __stdcall Release();


    // IPnpCallbackHardware
    //
    virtual HRESULT __stdcall OnPrepareHardware(_In_ IWDFDevice* pDevice);
    virtual HRESULT __stdcall OnReleaseHardware(_In_ IWDFDevice* pDevice);
      
    //
    // TODO: Add your interfaces here
    //
    
private:

    LONG m_cRefs;
    
};

