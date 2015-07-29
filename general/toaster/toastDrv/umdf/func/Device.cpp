/*++

  Copyright (c) Microsoft Corporation, All Rights Reserved

  Module Name:

    Device.cpp

  Abstract:

    This file contains the device callback object implementation.

  Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/
#include "stdafx.h"
#include "Device.h"

#include "internal.h"
#include "device.tmh"

HRESULT
CDevice::QueryInterface(
     _In_  REFIID riid,
     _Out_ LPVOID* ppvObject
    )
/*++

Routine Description:

    The framework calls this function to determine which callback
    interfaces we support.

Arguments:

    riid        - GUID for a given callback interface.
    ppvObject   - We set this pointer to our object if we support the
                  interface indicated by riid.

Return Value:

   HRESULT S_OK - Interface is supported.

--*/
{
    if (ppvObject == NULL)
    {
        return E_INVALIDARG;
    }

    *ppvObject = NULL;

    if ( riid == _uuidof(IUnknown) )
    {
        *ppvObject = static_cast<IPnpCallbackHardware*>(this);
    }    
    else if ( riid == _uuidof(IPnpCallbackHardware) )
    {
        *ppvObject = static_cast<IPnpCallbackHardware *>(this);
    }	
    else
    {
        return E_NOINTERFACE;
    }

    this->AddRef();

    return S_OK;
}


ULONG CDevice::AddRef()
/*++

Routine Description:

    Increments the ref count on this object.

Arguments:

    None.

Return Value:

    ULONG - new ref count.

--*/
{
    LONG cRefs = InterlockedIncrement( &m_cRefs );

    return cRefs;
}


_At_(this, __drv_freesMem(object))
ULONG CDevice::Release()
/*++

Routine Description:

    Decrements the ref count on this object.

Arguments:

    None.

Return Value:

    ULONG - new ref count.

--*/
{
    LONG cRefs;

    cRefs = InterlockedDecrement( &m_cRefs );

    if( 0 == cRefs )
    {
        delete this;
    }

    return cRefs;
}


HRESULT
CDevice::OnPrepareHardware(
     _In_  IWDFDevice* pDevice)
/*++

Routine Description:

    The framework calls this function after IDriverEntry::OnDeviceAdd
    returns and before the device enters the working power state.
    This callback prepares the device and the driver to enter the working 
    state after enumeration.

Arguments:

    pWdfDevice - A pointer to the IWDFDevice interface for the device 
    object of the device to make accessible. 

Return Value:

    S_OK in case of success
    HRESULT correponding to one of the error codes that are defined in Winerror.h.
    
--*/
{
    PWSTR deviceName = NULL;
    DWORD deviceNameCch = 0;

    HRESULT hr;

    Trace(TRACE_LEVEL_INFORMATION,"%!FUNC!");

    //
    // Get the device name.
    // Get the length to allocate first
    //

    hr = pDevice->RetrieveDeviceName(NULL, &deviceNameCch);
    
    //
    // Allocate the buffer
    //

    if (SUCCEEDED(hr))
    {
        deviceName = new WCHAR[deviceNameCch];

        if (deviceName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Get the actual name
    //

    if (SUCCEEDED(hr))
    {
        hr = pDevice->RetrieveDeviceName(deviceName, &deviceNameCch);
        
    }
  
    //
    // Do your hardware operations here
    // 

   delete[] deviceName;

   return hr;
}

HRESULT
CDevice::OnReleaseHardware(
     _In_ IWDFDevice* /*pDevice*/)
/*++

Routine Description:

    This routine is invoked when the device is being removed or stopped
    It releases all resources allocated for this device. The framework
    calls this callback after the device exits from the working power 
    state but before its queues are purged.


Arguments:

    pWdfDevice - A pointer to the IWDFDevice interface for the device object
    of the device that is no longer accessible. 


Return Value:
    HRESULT - Always succeeds.
--*/
{
    Trace(TRACE_LEVEL_INFORMATION,"%!FUNC!");

    return S_OK;
}



