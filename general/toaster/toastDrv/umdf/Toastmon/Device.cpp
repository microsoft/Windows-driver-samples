/*++
 
Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Device.cpp

Abstract:

    This module contains the implementation of the UMDF sample driver's
    device callback object.

    This sample demonstrates how to register for PnP event notification
    for an interface class, and how to handle arrival events.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "device.tmh"

HRESULT
CMyDevice::CreateInstance(
    _In_  IWDFDriver           * FxDriver,
    _In_  IWDFDeviceInitialize * FxDeviceInit,
    _Out_ PCMyDevice           * MyDevice
    )
/*++

  Routine Description:

    This method creates and initializs an instance of the driver's
    device callback object.

  Arguments:

    FxDeviceInit - the settings for the device.

    MyDevice - a location to store the referenced pointer to the device object.

  Return Value:

    Status

--*/
{
    PCMyDevice myDevice;
    HRESULT hr;

    //
    // Allocate a new instance of the device class.
    //

    myDevice = new CMyDevice();

    if (NULL == myDevice)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize the instance.
    //

    hr = myDevice->Initialize(FxDriver, FxDeviceInit);

    if (SUCCEEDED(hr))
    {
        *MyDevice = myDevice;
    }
    else
    {
        myDevice->Release();
    }

    return hr;
}

HRESULT
CMyDevice::Initialize(
    _In_ IWDFDriver           * FxDriver,
    _In_ IWDFDeviceInitialize * FxDeviceInit
    )
/*++
 
  Routine Description:

    This method initializes the device callback object and creates the
    partner device object.

    Then it registers for toaster device notifications.


  Arguments:

    FxDeviceInit - the settings for this device.

  Return Value:

    status.

--*/
{
    CComPtr<IWDFDevice> fxDevice;
    HRESULT hr;

    //
    // Save a weak reference to the Fx driver object. We'll need it to create
    // CMyRemoteTarget objects.
    //
    
    m_FxDriver = FxDriver;

    //
    // QueryIUnknown references the IUnknown interface that it returns
    // (which is the same as referencing the device).  We pass that to 
    // CreateDevice, which takes its own reference if everything works.
    //

    {
        IUnknown *unknown = this->QueryIUnknown();

        //
        // Create a new FX device object and assign the new callback object to 
        // handle any device level events that occur.
        //
        hr = FxDriver->CreateDevice(FxDeviceInit, unknown, &fxDevice);

        unknown->Release();
    }

    //
    // If that succeeded then set our FxDevice member variable.
    //

    CComPtr<IWDFDevice2> fxDevice2;
    if (SUCCEEDED(hr))
    {
        //
        // Q.I. for the latest version of this interface.
        //
        hr = fxDevice->QueryInterface(IID_PPV_ARGS(&fxDevice2));
    }

    if (SUCCEEDED(hr))
    {
        //
        // Store a weak reference to the IWDFDevice2 interface. Since this object
        // is partnered with the framework object they have the same lifespan - 
        // there is no need for an additional reference.
        //

        m_FxDevice = fxDevice2;
    }

    return hr;
}

HRESULT
CMyDevice::Configure(
    VOID
    )
/*++
 
  Routine Description:

    This method is called after the device callback object has been initialized 
    and returned to the driver.

  Return Value:

    status

--*/
{
    HRESULT hr = S_OK;
        
    //
    // Register for TOASTER device interface change notification.
    // We will get OnRemoteInterfaceArrival() calls when a remote toaster
    // device is started.
    //
    // Arrival notification will be sent for all existing and future toaster 
    // devices.
    //
    // The framework will take care of unregistration when the device unloads.
    //
    hr = m_FxDevice->RegisterRemoteInterfaceNotification(&GUID_DEVINTERFACE_TOASTER,
                                                         true);

    return hr;
}


HRESULT
CMyDevice::QueryInterface(
    _In_ REFIID InterfaceId,
    _Out_ PVOID *Object
    )
/*++

  Routine Description:

    This method is called to get a pointer to one of the object's callback
    interfaces.

  Arguments:

    InterfaceId - the interface being requested

    Object - a location to store the interface pointer if successful

  Return Value:

    S_OK or E_NOINTERFACE

--*/
{
    HRESULT hr;

    if(IsEqualIID(InterfaceId, __uuidof(IPnpCallbackRemoteInterfaceNotification))) 
    {    
        *Object = QueryIPnpCallbackRemoteInterfaceNotification();
        hr = S_OK;  
    }
    else
    {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }
    
    return hr;
}

//
// IPnpCallbackRemoteInterfaceNotification
//

void 
STDMETHODCALLTYPE
CMyDevice::OnRemoteInterfaceArrival(
    _In_ IWDFRemoteInterfaceInitialize * FxRemoteInterfaceInit
    )
/*++

  Routine Description:

    This method is called by the framework when a new remote interface has come
    online. These calls will only occur one at a time.

  Arguments:

    FxRemoteInterfaceInit - An identifier for the remote interface.

--*/
{
    HRESULT hr = S_OK;

    
    //
    // Create a new FX remote interface object and assign a NULL callback 
    // object since we don't care to handle any remote interface level events 
    // that occur.
    //
    
    CComPtr<IWDFRemoteInterface> fxRemoteInterface;

    hr = m_FxDevice->CreateRemoteInterface(FxRemoteInterfaceInit, 
                                           NULL, 
                                           &fxRemoteInterface);
    
    //
    // Create an instance of CMyRemoteTarget which will open the remote device
    // and post I/O requests to it.
    //

    PCMyRemoteTarget myRemoteTarget = NULL;
    if (SUCCEEDED(hr))
    {
        hr = CMyRemoteTarget::CreateInstance(this,
                                             m_FxDriver, 
                                             m_FxDevice, 
                                             fxRemoteInterface,
                                             &myRemoteTarget);
    }

    if (SUCCEEDED(hr))
    {
        if (myRemoteTarget != NULL)
        {
            //
            // Add to our list
            //
            InsertHeadList(&m_MyRemoteTargets, &myRemoteTarget->m_Entry);
            
            //
            // Release, since framework will keep a reference
            //
            myRemoteTarget->Release();
        }
    }
    
    if (FAILED(hr))
    {
        if (fxRemoteInterface != NULL)
        {
            //
            // We failed to create the CMyRemoteTarget, delete the 
            // RemoteInterface object
            //
            fxRemoteInterface->DeleteWdfObject();
        }
    }
}

